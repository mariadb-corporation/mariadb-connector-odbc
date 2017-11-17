#!/bin/bash

set -x
set -e

###################################################################################################################
# test different type of configuration
###################################################################################################################
mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=3305 )
export COMPOSE_FILE=.travis/docker-compose.yml



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
    echo 'SELECT 1' | "${mysql[@]}"
    echo >&2 'data server init process failed.'
    exit 1
fi

#list ssl certificates
ls -lrt ${SSLCERT}


#build C connector
DEBIAN_FRONTEND=noninteractive sudo apt-get install --allow-unauthenticated -y --force-yes -m unixodbc-dev
#mkdir ./connector_c
time git clone -b master --depth 1 "https://github.com/MariaDB/mariadb-connector-c.git" build
cd build
#git fetch --all --tags --prune
#git checkout tags/${CONNECTOR_C_VERSION} -b branch_odbc

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DWITH_OPENSSL=ON
#-DCMAKE_INSTALL_PREFIX= ../connector_c .
make
sudo make install
cd ..
rm build -rf

#build odbc connector
export TEST_DRIVER=maodbc_test
export TEST_DSN=maodbc_test
export TEST_PORT=3305
export TEST_SERVER=mariadb.example.com
export TEST_SOCKET=
export TEST_SCHEMA=odbc_test
export TEST_UID=bob
export TEST_PASSWORD= 

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DMARIADB_DIR=./connector_c .
cmake --build . --config RelWithDebInfo 

###################################################################################################################
# run test suite
###################################################################################################################
echo "Running tests"

cd test
export ODBCINI="$PWD/odbc.ini"
export ODBCSYSINI=$PWD

export MAODBCTESTS_IN_TRAVIS=1

#Just to know our env
export

ctest -V
