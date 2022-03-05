for id in $(docker ps -q)
do
  docker stop "$id"
done

# Use this for PowerShell!
docker system prune --force