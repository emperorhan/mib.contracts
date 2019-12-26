#!/bin/bash

WALLETHOST="127.0.0.1"
NODEHOST="test.ledgis.io"
NODEPORT="80"
WALLETPORT="7777"

docker exec -it ledgis cleos -u http://$NODEHOST:$NODEPORT --wallet-url http://$WALLETHOST:$WALLETPORT "$@"
# docker exec -it ledgis cleos -u http://$NODEHOST:$NODEPORT --wallet-url http://$WALLETHOST:$WALLETPORT "$@"



