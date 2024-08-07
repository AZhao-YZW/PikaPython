#!/bin/bash

# Define the local directory and the container directory
LOCAL_DIR="/home/guest/project/Leaf_python/PikaPython"
CONTAINER_DIR="/home"
CONTAINER_NAME="leafdev"
IMAGE_NAME="devenv"

# Check if the image exists
if [[ "$(docker images -q ${IMAGE_NAME} 2> /dev/null)" == "" ]]; then
    echo "Image ${IMAGE_NAME} does not exist. Building it..."
    docker buildx build . -t ${IMAGE_NAME}
fi

if [ "$(docker ps -aq -f name=${CONTAINER_NAME})" ]; then
    echo "Container ${CONTAINER_NAME} exists."
    
    # Check if the container is running
    if [ "$(docker ps -q -f name=${CONTAINER_NAME})" ]; then
        echo "Container ${CONTAINER_NAME} is already running. Attaching to it..."
        docker exec -it ${CONTAINER_NAME} /bin/bash
    else
        echo "Container ${CONTAINER_NAME} is not running. Starting it..."
        docker start ${CONTAINER_NAME}
        docker exec -it ${CONTAINER_NAME} /bin/bash
    fi
else
    echo "Container ${CONTAINER_NAME} does not exist. Creating and starting it..."
    docker run --name ${CONTAINER_NAME} -v ${LOCAL_DIR}:${CONTAINER_DIR} -itd ${IMAGE_NAME} /bin/bash
fi
