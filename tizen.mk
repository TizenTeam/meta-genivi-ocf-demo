#! /usr/bin/make -f

default:all

package?=mapo


all?= tmp/512x512.png \
 shared/res/iotivityexample.png

all: ${all}

icon.png: docs/logo.png Makefile
	convert -resize 117x117! $< $@

docs/screenshot.png: docs/logo.png
	echo cp $< $@

tmp/screenshot.jpg: docs/screenshot.png
	convert $< $@

tmp/117x177/%: %
	mkdir -p ${@D}
	convert -resize '117x117!' $< $@


tmp/512x512.png: docs/logo.png
	mkdir -p ${@D}
	convert -resize '512x512!' $< $@

tmp/512x512.jpg: docs/logo.png
	mkdir -p ${@D}
	convert -resize '512x512!' $< $@

tmp/480x854/gmaps.png: docs/gmaps.png
	mkdir -p ${@D}
	convert -resize '480x854!' $< $@

tmp/480x854/openlayers-gmaps.png: docs/openlayers-gmaps.png
	mkdir -p ${@D}
	convert -resize '480x854!' $< $@

distclean: clean
	rm -f *.wgt
	rm -rf Debug Release
	rm -rf .package-stamp .settings .sign
	rm -rf crash-info SA_Report .sdk_delta.info
	rm -rf lib usr tmp

clean:
	rm -f *~

check: ${package}.wgt
	unzip -t $<

docs/logo.jpg:
	mkdir -p $@{D}
	wget -O $@.tmp http://research.cisco.com/img/devnet-page/tiles/iotivity.jpg
	convert $@.tmp $@


docs/logo.png: docs/logo.jpg
	convert $< $@

shared/res/iotivityexample.png: tmp/117x177/docs/logo.png
	cp $< $@
