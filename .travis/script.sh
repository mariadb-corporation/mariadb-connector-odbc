#!/bin/bash

set -x
set -e

###################################################################################################################
# test different type of configuration
###################################################################################################################
mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=3305 )
export COMPOSE_FILE=.travis/docker-compose.yml


if [ -n "$MAXSCALE_VERSION" ]
then
  mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=4007 )
  export COMPOSE_FILE=.travis/maxscale-compose.yml
  export TEST_PORT=4007
else
  export TEST_PORT=3305
fi


###################################################################################################################
# launch docker server and maxscale
###################################################################################################################
export INNODB_LOG_FILE_SIZE=$(echo ${PACKET}| cut -d'M' -f 1)0M
docker-compose -f ${COMPOSE_FILE} build
docker-compose -f ${COMPOSE_FILE} up -d


###################################################################################################################
# wait for docker initialisation
###################################################################################################################

for i in {60..0}; do
    if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
        break
    fi
    echo 'data server still not active'
    sleep 1
done

docker-compose -f ${COMPOSE_FILE} logs

if [ "$i" = 0 ]; then
#    echo 'SELECT 1' | "${mysql[@]}"
    set +e
    echo 'SELECT 1' | "${mysql[@]}" --ssl-ca=$SSLCERT/server.crt --ssl-key=$SSLCERT/client.key --ssl-cert=$SSLCERT/client.crt
    echo >&2 'data server init process failed.'
    set -e
#    exit 1
fi

#list ssl certificates
ls -lrt ${SSLCERT}


DEBIAN_FRONTEND=noninteractive sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get install --allow-unauthenticated -y --force-yes -m unixodbc-dev odbcinst1debian2 libodbc1 

#build odbc connector
export TEST_DRIVER=maodbc_test
export TEST_DSN=maodbc_test
export TEST_SERVER=mariadb.example.com
export TEST_SOCKET=
export TEST_SCHEMA=odbc_test
export TEST_UID=bob
export TEST_PASSWORD= 

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL
# In Travis we are interested in tests with latest C/C version, while for release we must use only latest release tag
#git submodule update --remote
cmake --build . --config RelWithDebInfo 

###################################################################################################################
# run test suite
###################################################################################################################
echo "Running tests"

cd test
export ODBCINI="$PWD/odbc.ini"
export ODBCSYSINI=$PWD


ctest -V

