#!/usr/bin/env bash

set -e

if ! [ -x "$(command -v pip)" ]; then
  curl -O https://bootstrap.pypa.io/get-pip.py

  if [ -v PYTHON_BIN_PATH ]; then
    PYTHON_BIN="$PYTHON_BIN_PATH"
  else
    PYTHON_BIN="python"
  fi
  "$PYTHON_BIN" --version
  "$PYTHON_BIN" get-pip.py --user
fi

pip install numpy $1
pip install empy $1  # For ROS C++ message generation
pip install pyyaml $1  # For ROS Python message generation