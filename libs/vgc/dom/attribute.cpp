// Copyright 2021 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vgc/dom/attribute.h>

/*

Implementation notes:

I am still a bit undecided on the C++ API and implementation to manipulate
 attributes in the dom. Here are some alternatives:

#1 all dynamic
--------------

For each element, each of its attributes, even non-authored built-in
attributes, is a dynamically allocated object (e.g., a core::Object, but not
necessarily).

Advantages: Cleaner API, consistent with "Document/Element", allows clients to
retain weak pointers to attributes and have fast access to their values,
listening when the value changes, etc.

Drawbacks: Possibly a lot of memory wasted, and all those dynamic allocations
might not be very cache-friendly.

#2 private authored dynamic
---------------------------

Only dynamically allocate AuthoredAttributes, but do not expose this type.
Attributes access and authoring is done via element->getAttribute(name) and
element->setAttribute(name, value).

Optionally, we can have a public "Attribute" class that only stores its owner
Element* and the attribute's name, and is merely syntactic sugar for invoking
element API.

Advantages: This saves a lot of memory: only what's authored is actually
allocated.

Drawbacks: reading/authoring attributes is slower: the element always has to
"find" the attribute for any operation. Clients cannot keep a pointer to the
internal AuthoredAttribute* for fast access to where the data live, or listening
to changes

#3 public authored dynamic
--------------------------

Only dynamically allocate authored attributes (same as #2), but have
"Attribute" keep a pointer to the internal AuthoredAttribute* if any, and to
the global BuiltinAttribute* if any.

Advantages: Same memory efficiency as #2, but also allows clients to retain
pointers to where the data live, for either fast access or listening to
changes (only possible in case of an authored attribute).

Inconvenient: slightly unsafe: what if the requested attribute is first
authored, but authoring is cleared later, possibly in another thread?
Or possibly trickier: what if a built-in attribute gets authored?

Attribute attr1 = element->attribute(name); // suppose it's not authored yet
Attribute attr2 = element->attribute(name); // suppose it's not authored yet
attr1.setValue(v1); // this dynamically allocates an AuthoredAttribute.

attr2.getValue();   // How does attr2 knows that v1 was already authored?
                    // How to get the corresponding AuthoredAttribute?

attr2.setValue(v2); // How does attr2 knows that v1 was already authored?

#4 on-demand dynamic
--------------------

By default, only dynamically allocate authored attributes (same as #2), and do
not store in Attribute the pointers to internal AuthoredAttribute* or
BuiltinAttribute* (also same as #2). However, for clients who desire to have
fast successive access, or desire to listen to changes in a safe way, allow
them to get a pointer to a dynamically allocated "OnDemandAttribute*".

In other word, by default do not perform dynamic allocation for non-authored
built-in attribute, but let clients perform on-demand dynamic allocation.

Implementations may vary, but one option is:

class Element {
    // ...
    std::vector<AuthoredAttributePtr> authored_;
    std::vector<OnDemandAttributePtr> onDemand_;
    static std::vector<BuiltInAttributePtr> builtIn_;
};

class Attribute {
    // ...
    OnDemandAttributeWeakPtr onDemand() const;
};

Having clients keep shared pointers of on-demand attributes, and elements have
weak pointers sounds rationale too at first sight, but what if the element is
deleted? The on-demand attribute should expire too, therefore clients can't
hold a shared pointer to them.

Conclusion
----------

Unfortunately, benchmarking to determine which approach is best cannot yet be
done, since there are too many non-implemented features that would affect the
benchmarking. Therefore, for the time being, I simply decided on #2 using my
intuition, and because it can easily be extended into #4 later. Approach #1 may
be best, who knows, and it would be the go-to approach if I strictly followed
the "premature optimization is the root of all evil" adage. However, in this
case, my gut feeling makes me scared of scalability issues, and it would be
really hard to change from #1 to #2 later on, once we discover those
scalability problems.

*/
