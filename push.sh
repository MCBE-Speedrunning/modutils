#!/usr/bin/env sh

# Push to both gitea and the github mirror

git push -u origin master
git push -u github master
