--local x = 0

function init()
	io.write("init \n")
	return 0
end

function update() -- called once per frame
    --x = x + 1;
    --io.write(x .. " update \n")

    local dt = time.deltaTime

    --transform.positionX = transform.positionX + 10.0 * dt

    --transform.rotationX = transform.rotationX - 30.0 * dt
    --transform.rotationY = transform.rotationY - 15.0 * dt
    transform.rotationZ = transform.rotationZ + 120.0 * dt

    --io.write(transform.position.x .. " positionX \n")
    --io.write(transform.position.y .. " positionY \n")
    --io.write(transform.position.z .. " positionZ \n")
    --io.write(time.deltaTime .. " deltatime \n")
    --io.write(time.lastFrameTime .. " lastFrameTime \n")
    --io.write(time.frameRate .. " frameRate \n")
    --io.write(time.time .. " time \n")

    --io.stdout:flush()
	return 0
end