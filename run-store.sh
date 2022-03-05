if [[ $# != 1 ]]
then
    echo "Usage: ./run-store <host_name>"
    exit 1
fi

mkdir -p "volumes/store/$1"

docker build -f "dockerfiles/Store" -t "store:latest" ./sources

docker run -it --network="host" --hostname "$1" -v "$(pwd)/volumes/store/$1":/store/data store:latest