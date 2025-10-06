# MemoryManager
## Project Overview
- In this project, I implemented the Small Object Allocator by Andrei Alexandrescu (Modern C++ Design: Generic Programming and Design Patterns Applied - 2001).
- I propose a small variation to optimize the butterfly allocation trend and in general improve allocation/deallocation time.
- I tried to speed up allocation by tracking both full and completely empty chunks. When the current "allocChunk" is full, the allocator first checks if there is an empty chunk available for immediate reuse; if all chunks are full, a new chunk is allocated. If neither of these options applies, it means partially filled chunks are in the pool, so the search begins there.
- For deallocation I use std::deque to preserve stable pointers, which allows me to maintain a map (chunk pData - pointer to owning chunk. This enables quick location of the correct chunk during deallocation.
- This project allowed me to explore memory management and allocation strategies in C++, sharpening my understanding through experimentation.
## Results
![ResultsImage](ButterflyTrendTest)
