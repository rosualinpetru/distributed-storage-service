docker build -f "dockerfiles/Client" -t "client:latest" ./sources

docker run -it --network="host" -v "$(pwd)/sources/tests":/client/data client:latest