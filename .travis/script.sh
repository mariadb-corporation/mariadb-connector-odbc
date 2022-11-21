#!/bin/bash

set -ex

#if [ -n "$BENCH" ] ; then
#  benchmark/build.sh
#  cd benchmark
#  ./installation.sh
#  ./launch.sh
#  exit
#fi

# Setting test environment before building connector to configure tests default credentials
if [ "$TRAVIS_OS_NAME" = "windows" ] ; then
  echo "build from windows"
  ls -l
  if [ -e ./settestenv.sh ] ; then
    source ./settestenv.sh
  fi
else
  echo "build from linux"
  export SSLCERT=$TEST_DB_SERVER_CERT
  export MARIADB_PLUGIN_DIR=$PWD

  export SSLCERT=$TEST_DB_SERVER_CERT
  if [ -n "$MYSQL_TEST_SSL_PORT" ] ; then
    export TEST_SSL_PORT=$MYSQL_TEST_SSL_PORT
  fi
fi

export TEST_DSN=maodbc_test
export TEST_DRIVER=maodbc_test
export TEST_UID=$TEST_DB_USER
export TEST_SERVER=$TEST_DB_HOST
export TEST_PASSWORD=$TEST_DB_PASSWORD
export TEST_PORT=$TEST_DB_PORT
export TEST_SCHEMA=testo
export TEST_VERBOSE=true
export TEST_SOCKET=
if [ "${TEST_REQUIRE_TLS}" = "1" ] ; then
  export TEST_USETLS=true
  export TEST_ADD_PARAM="FORCETLS=1"
fi

sudo apt install cmake

if [ "$TRAVIS_OS_NAME" = "windows" ] ; then
  cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DWITH_MSI=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=SCHANNEL .
else
  if [ "$TRAVIS_OS_NAME" = "osx" ] ; then
    export TEST_DRIVER="$PWD/libmaodbc.dylib"
    cmake -G Xcode -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=OPENSSL -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DWITH_EXTERNAL_ZLIB=On .
  else
    cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=OPENSSL .
  fi
fi

set -x
cmake --build . --config RelWithDebInfo 

if [ -n "$server_branch" ] ; then

  ###################################################################################################################
  # Building server for testing
  ###################################################################################################################
  echo "Testing against built from the source server is not supported at the moment"
  exit 1

  # change travis localhost to use only 127.0.0.1
  sudo sed -i 's/127\.0\.1\.1 localhost/127.0.0.1 localhost/' /etc/hosts
  sudo tail /etc/hosts

  # get latest server
  git clone -b ${server_branch} https://github.com/mariadb/server ../workdir-server --depth=1

  cd ../workdir-server
  export SERVER_DIR=$PWD

  # don't pull in submodules. We want the latest C/C as libmariadb
  # build latest server with latest C/C as libmariadb
  # skip to build some storage engines to speed up the build

  mkdir bld
  cd bld
  cmake .. -DPLUGIN_MROONGA=NO -DPLUGIN_ROCKSDB=NO -DPLUGIN_SPIDER=NO -DPLUGIN_TOKUDB=NO
  echo "PR:${TRAVIS_PULL_REQUEST} TRAVIS_COMMIT:${TRAVIS_COMMIT}"
  if [ -n "$TRAVIS_PULL_REQUEST" ] && [ "$TRAVIS_PULL_REQUEST" != "false" ] ; then
    # fetching pull request
    echo "fetching PR"
  else
    echo "checkout commit"
  fi

  cd $SERVER_DIR/bld
  make -j9

fi
###################################################################################################################
# run connector test suite
###################################################################################################################
echo "run connector test suite"

cd ./test
export ODBCINI="$PWD/odbc.ini"
export ODBCSYSINI=$PWD

cat $ODBCINI

ctest --output-on-failure
# Running tests 2nd time with resultset streaming. "${TEST_REQUIRE_TLS}" = "1" basically means "not on skysql"
if ! [ "${TEST_REQUIRE_TLS}" = "1" ] && ! [ "$srv" = "xpand" ]; then
  export TEST_ADD_PARAM="STREAMRS=1;FORWARDONLY=1"
  ctest --output-on-failure
fi

