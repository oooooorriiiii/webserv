#!/bin/bash

sudo rm /etc/nginx/conf.d/*.conf
sudo cp config/nginxInception.conf /etc/nginx/conf.d/
sudo rm -rf /var/www/inception_server
sudo rm -rf /var/www/webserv
sudo cp -R ./var/www/webserv /var/www/