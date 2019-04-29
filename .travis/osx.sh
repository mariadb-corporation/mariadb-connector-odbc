#!/bin/bash

set -x
set -e

# set variables for Connector/ODBC
export TEST_DRIVER="$PWD/libmaodbc.dylib"
export TEST_DSN=madb_connstring_test
export TEST_SERVER=localhost
export TEST_SOCKET=
export TEST_SCHEMA=odbc_test
export TEST_UID=root
export TEST_PASSWORD= 

# for some reason brew upgrades postgresql, so let's remove it
# brew remove postgis
# brew uninstall --ignore-dependencies postgresql
# upgrade openssl
# brew upgrade openssl
# install unixodbc
# brew install libiodbc
# install and start MariaDB Server
# brew install mariadb
mysql.server start

# ls -la /usr/local/Cellar/openssl/
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL -DWITH_IODBC=ON -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2o_2 -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2o_2/lib
cmake --build . --config RelWithDebInfo

###################################################################################################################
# run test suite
###################################################################################################################

# set variables for odbc.ini and odbcinst.ini
export ODBCINI="$PWD/test/odbc.ini"
cat ${ODBCINI}
export ODBCINSTINI="$PWD/test/odbcinst.ini"
cat ${ODBCINSTINI}

# check users of MariaDB and create test database
mysql --version
mysql -u root -e "SELECT user, host FROM mysql.user"
mysql -u root -e "CREATE DATABASE odbc_test"
mysql -u root -e "SHOW DATABASES"

echo "Running tests"
cd test
ctest -V
