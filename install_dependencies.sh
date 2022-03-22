#!/bin/sh

if test ! -d Linksimulator; then
	git clone https://github.com/cnp3/Linksimulator && \
	cd Linksimulator && \
  make && \
	cp link_sim ..
else
  exit 0
fi