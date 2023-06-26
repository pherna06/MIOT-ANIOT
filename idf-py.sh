CURRENTDIR=$(pwd)

# Docker Command
DOCKER_CMD="docker run"
DOCKER_CMD_ARGS="--rm -it -v $CURRENTDIR:/project -w /project"
DOCKER_BASE_IMG="espressif/idf"
DOCKER_IMG="$DOCKER_BASE_IMG:latest" # default image

# idf.py Command
IDF_PY="idf.py"
IDF_PY_ARGS=""
# process arguments
while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
        -t|--tag)
        DOCKER_IMG="$DOCKER_BASE_IMG:$2"

        if   [[ $2 == "release-v4."* ]]; then
            DOCKER_CMD_ARGS="$DOCKER_CMD_ARGS -e CMAKE_VER=3.5"
        elif [[ $2 == "release-v5."* ]]; then
            DOCKER_CMD_ARGS="$DOCKER_CMD_ARGS -e CMAKE_VER=3.16"
        else
            DOCKER_CMD_ARGS="$DOCKER_CMD_ARGS -e CMAKE_VER=3.5"
        fi
        shift # past argument
        shift # past value
        ;;

        -p|--port) # add device to docker run
        DOCKER_CMD_ARGS="$DOCKER_CMD_ARGS --device=$2"
        shift # past argument
        shift # past value
        ;;

        *)    # add to idf.py args
        IDF_PY_ARGS="$IDF_PY_ARGS $1"
        shift # past argument
        ;;
    esac
done

# Run idf.py command
MAKE="$DOCKER_CMD $DOCKER_CMD_ARGS $DOCKER_IMG $IDF_PY $IDF_PY_ARGS"
echo "> Running: $MAKE"

$MAKE