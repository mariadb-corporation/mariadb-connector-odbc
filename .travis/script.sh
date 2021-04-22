#!/bin/bash

set -x
set -e

###################################################################################################################
# test different type of configuration
###################################################################################################################

export TEST_SOCKET=
export ENTRYPOINT=$PROJ_PATH/.travis/sql
export ENTRYPOINT_PAM=$PROJ_PATH/.travis/pam
export TEST_DSN=maodbc_test

set +x

if [ -n "$SKYSQL" ] || [ -n "$SKYSQL_HA" ]; then
  if [ -n "$SKYSQL" ]; then
    ###################################################################################################################
    # test SKYSQL
    ###################################################################################################################
    if [ -z "$SKYSQL_HOST" ] ; then
      echo "No SkySQL configuration found !"
      exit 0
    fi

    export TEST_UID=$SKYSQL_USER
    export TEST_SERVER=$SKYSQL_HOST
    export TEST_PASSWORD=$SKYSQL_PASSWORD
    export TEST_PORT=$SKYSQL_PORT

  else

    ###################################################################################################################
    # test SKYSQL with replication
    ###################################################################################################################
    if [ -z "$SKYSQL_HA" ] ; then
      echo "No SkySQL HA configuration found !"
      exit 0
    fi

    export TEST_UID=$SKYSQL_HA_USER
    export TEST_SERVER=$SKYSQL_HA_HOST
    export TEST_PASSWORD=$SKYSQL_HA_PASSWORD
    export TEST_PORT=$SKYSQL_HA_PORT
  fi
  # Common for both SKYSQL testing modes
  export TEST_SCHEMA=testo
  export TEST_USETLS=1
  export TEST_ADD_PARAM="FORCETLS=1"

else
  export COMPOSE_FILE=.travis/docker-compose.yml

  export TEST_DRIVER=maodbc_test
  export TEST_SCHEMA=odbc_test
  export TEST_UID=bob
  export TEST_SERVER=mariadb.example.com
  export TEST_PASSWORD=
  export TEST_PORT=3305

  if [ -n "$MAXSCALE_VERSION" ] ; then
      # maxscale ports:
      # - non ssl: 4006
      # - ssl: 4009
      export TEST_PORT=4006
      export TEST_SSL_PORT=4009
      export COMPOSE_FILE=.travis/maxscale-compose.yml
      docker-compose -f ${COMPOSE_FILE} build
  fi

  mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=3305 )

  ###################################################################################################################
  # launch docker server and maxscale
  ###################################################################################################################
  docker-compose -f ${COMPOSE_FILE} up -d


  ###################################################################################################################
  # wait for docker initialisation
  ###################################################################################################################
  for i in {30..0}; do
    if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
        break
    fi
    echo 'data server still not active'
    sleep 2
  done

  if [ "$i" = 0 ]; then
    if echo 'SELECT 1' | "${mysql[@]}" ; then
        break
    fi

    docker-compose -f ${COMPOSE_FILE} logs
    if [ -n "$MAXSCALE_VERSION" ] ; then
        docker-compose -f ${COMPOSE_FILE} exec maxscale tail -n 500 /var/log/maxscale/maxscale.log
    fi
    echo >&2 'data server init process failed.'
    exit 1
  fi

  if [[ "$DB" != mysql* ]] ; then
    ###################################################################################################################
    # execute pam
    ###################################################################################################################
    docker-compose -f ${COMPOSE_FILE} exec -u root db bash /pam/pam.sh
    sleep 1
    docker-compose -f ${COMPOSE_FILE} restart db
    sleep 5

    ###################################################################################################################
    # wait for restart
    ###################################################################################################################

    for i in {30..0}; do
      if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
          break
      fi
      echo 'data server restart still not active'
      sleep 2
    done

    if [ "$i" = 0 ]; then
      if echo 'SELECT 1' | "${mysql[@]}" ; then
          break
      fi

      docker-compose -f ${COMPOSE_FILE} logs
      if [ -n "$MAXSCALE_VERSION" ] ; then
          docker-compose -f ${COMPOSE_FILE} exec maxscale tail -n 500 /var/log/maxscale/maxscale.log
      fi
      echo >&2 'data server restart process failed.'
      exit 1
    fi
  fi

  #list ssl certificates
  ls -lrt ${SSLCERT}
fi

DEBIAN_FRONTEND=noninteractive sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get install --allow-unauthenticated -y --force-yes -m unixodbc-dev odbcinst1debian2 libodbc1 

#build odbc connector

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


ctest -VV

