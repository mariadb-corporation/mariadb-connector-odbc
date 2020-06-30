#!/bin/bash
# ************************************************************************************
#   Copyright (C) 2019 MariaDB Corporation AB
#   
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#   
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#   
#   You should have received a copy of the GNU Library General Public
#   License along with this library; if not see <http://www.gnu.org/licenses>
#   or write to the Free Software Foundation, Inc., 
#   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
# *************************************************************************************/

LibPath="Library/MariaDB/MariaDB-Connector-ODBC"

set -e
rm -rf ./ROOT

mkdir -p ./ROOT/${LibPath}/bin
cp $1/libmaodbc.dylib ./ROOT/${LibPath}/
cp ./install_driver ./ROOT/${LibPath}/bin

if [ $2 ]; then
  mkdir ./ROOT/${LibPath}/plugin
  cp $2/dialog.so ./ROOT/${LibPath}/plugin/
  cp $2/auth_gssapi_client.so ./ROOT/${LibPath}/plugin/
  cp $2/caching_sha2_password.so ./ROOT/${LibPath}/plugin/
  cp $2/mysql_clear_password.so ./ROOT/${LibPath}/plugin/
  cp $2/client_ed25519.so ./ROOT/${LibPath}/plugin/
  cp $2/sha256_password.so ./ROOT/${LibPath}/plugin/
fi

if [ $3 ]; then
  if [ -L $3 ]; then
    LibRealName=`readlink $3`
    cp $3 ./ROOT/${LibPath}/${LibRealName}
    cp -a $3 ./ROOT/${LibPath}/
  else
    cp $3 ./ROOT/${LibPath}/
  fi
fi
