#!/bin/bash
# sudo npm install -g uglify-js
# sudo npm install -g uglifycss
# sudo npm install -g html-minifier

uglifyjs "script.js" -o "../../data/script.js" --compress --mangle
uglifycss "style.css" > "../../data/style.css"
html-minifier --collapse-whitespace --remove-comments --remove-optional-tags --remove-redundant-attributes --remove-script-type-attributes --remove-tag-whitespace --use-short-doctype --minify-css true --minify-js true -o "../../data/index.html" index.html