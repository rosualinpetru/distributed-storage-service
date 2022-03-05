docker build -f "dockerfiles/Server" -t "server:latest" ./sources

mkdir -p volumes/server

docker run -it --network="host" -v "$(pwd)/volumes/server":/server/data/buckets server:latest
