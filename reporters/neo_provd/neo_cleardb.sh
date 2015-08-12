#!/bin/bash

neo4j stop
rm -rf /home/adam/neo4j/data/graph.db/*
neo4j start

echo ""

