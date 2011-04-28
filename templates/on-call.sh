#!/bin/bash

case "$1" in
  CALL)
    # handle incoming or outgoing calls
  ;;
  CONNECT)
    # handle call connects
  ;;
  DISCONNECT)
    # handle call disconnects
  ;;
  FINISHED)
    # all ongoing calls have been finished
    # if only one call has been handled, this is called directly after DISCONNECT
  ;;
  *)
    echo "This script should not be called directly."
    exit 1
  ;;
esac

exit 0
  