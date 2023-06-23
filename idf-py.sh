CURRENTDIR=$(pwd)

DOCKER_CMD="docker run --rm -it -v $CURRENTDIR:/project -w /project"
DOCKER_IMG="espressif/idf:release-v4.4"
IDF_PY="idf.py"

# Serial port
while getopts ":p:" opt; do
  case $opt in
    p)
        DOCKER_CMD="$DOCKER_CMD --device=$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
    ;;
  esac
done

# Run idf.py command
MAKE="$DOCKER_CMD $DOCKER_IMG $IDF_PY $@"
echo $MAKE

$MAKE