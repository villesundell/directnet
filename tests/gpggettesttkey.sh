#!/bin/sh
gpg --homedir ~/.dntest --delete-key Testt
gpg --homedir ~/.directnet -a --export Testt | gpg --homedir ~/.dntest --import
