#!/bin/bash

WALLETHOST="127.0.0.1"
NODEHOST="121.134.238.182"
NODEPORT="8011"
WALLETPORT="7777"

docker exec -it ledgis cleos -u http://$NODEHOST:$NODEPORT --wallet-url http://$WALLETHOST:$WALLETPORT "$@"
# docker exec -it ledgis cleos -u http://$NODEHOST:$NODEPORT --wallet-url http://$WALLETHOST:$WALLETPORT "$@"



