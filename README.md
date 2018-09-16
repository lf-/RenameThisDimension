# Rename This Dimension!

https://forums.autodesk.com/t5/fusion-360-ideastation/create-parameter-name-whilst-adding-a-sketch-dimension/idi-p/5443699

Date: 2014-12-15. Implemented: nope. That is all.

This implements the oft-demanded feature in the form of a button in the right click menu of a dimension.

## Usage

Select a sketch dimension, then in the right click menu, select Rename Dimension.

## Building

Requires Visual Studio 2017 Preview 15.6+ or so because of the use of /experimental:external.


Porting to Mac: you probably need to add extlibs to the include paths, and
that's about it.