#!/bin/bash

./build/nex test \
    --start-pc 0xC000 \
    --stop-pc 0xC66E \
    --assert-mem 0x02=0x00 \
