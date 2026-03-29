---
name: documentation-conventions
description: Documentation conventions for C++ code
---

When documenting code:
1. use /\* \*/ syntax
1. If the code can be connected to content in $PROJECTROOT/doc/worldview_and_mechanics, prefer write doc describing the connection with that, with the corresponding *.md at top. If not, then write the documentation normally.

When updating documentation at $PROJECTROOT/doc/worldview_and_mechanics:

3. If the mechanic have a corresponding code, write [Implemented in Foo::Bar]. If tied to a specific label denoted block in a function, write [Implemented in Foo::Bar#label].