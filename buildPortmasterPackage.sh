#!/bin/sh
cd .build
mkdir tinamp
cp -r ../package/* tinamp/
cp -r libs tinamp/tinamp
cp tinamp_aarch64 tinamp/tinamp
cd tinamp
zip -r ../tinamp.zip *
