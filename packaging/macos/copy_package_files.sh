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
cp $2//install_driver ./ROOT/${LibPath}/bin

if [ $3 ]; then
  mkdir ./ROOT/${LibPath}/plugin
  cp $3/dialog.so ./ROOT/${LibPath}/plugin/
  cp $3/auth_gssapi_client.so ./ROOT/${LibPath}/plugin/
  cp $3/caching_sha2_password.so ./ROOT/${LibPath}/plugin/
  cp $3/mysql_clear_password.so ./ROOT/${LibPath}/plugin/
  cp $3/client_ed25519.so ./ROOT/${LibPath}/plugin/
  cp $3/sha256_password.so ./ROOT/${LibPath}/plugin/
fi

if [ $4 ]; then
  if [ -L $4 ]; then
    LibRealName=`readlink $4`
    cp $4 ./ROOT/${LibPath}/${LibRealName}
    cp -a $4 ./ROOT/${LibPath}/
  else
    cp $4 ./ROOT/${LibPath}/
  fi
fi
