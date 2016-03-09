#!/bin/bash
make
EINA_LOG_LEVEL=3 gdb ./gateway
