set -ex
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ls -d $SCRIPT_DIR/../databases/* | grep -xv ".gitkeep" | xargs rm -r