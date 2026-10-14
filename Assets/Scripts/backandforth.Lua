--local x = 0

function init()
    io.write("init \n")
    return 0
end

function update() -- called once per frame
    --x = x + 1;
    --io.write(x .. " update \n")

    local dt = time.deltaTime

    local speed = 15.0

    local elipson = 3.0

    if transform.positionZ > -60.0 then
    transform.positionZ = transform.positionZ - speed * dt;
--    elseif transform.positionX < camera.positionX - elipson then
--     transform.positionX = transform.positionX + speed * dt;
    end

    if transform.positionZ < -60.0 then
        transform.positionZ = 55.0;
    end


    --if transform.positionY > camera.positionY - 2.2 then
    --transform.positionY = transform.positionY - speed * dt;
    --elseif transform.positionY < camera.positionY - 2.2 then
    -- transform.positionY = transform.positionY + speed * dt;
    --end

    --if transform.positionZ > camera.positionZ + elipson then
    --transform.positionZ = transform.positionZ - speed * dt;
    --elseif transform.positionZ < camera.positionZ - elipson then
    -- transform.positionZ = transform.positionZ + speed * dt;
    --end





    --transform.positionX = transform.positionX + 40.0 * dt

    --transform.rotationX = transform.rotationX - 30.0 * dt
    --transform.rotationY = transform.rotationY - 120 * dt
    --transform.rotationZ = transform.rotationZ - 30.0 * dt
    --if transform.rotationX >= 360.0 or transform.rotationX <= -360.0 then
    --    transform.rotationX = 0.0;
    --end
    --if transform.rotationY >= 360.0 or transform.rotationY <= -360.0 then
    --    transform.rotationY = 0.0;
    --end
    --if transform.rotationZ >= 360.0 or transform.rotationZ <= -360.0 then
    --    transform.rotationZ = 0.0;
    --end
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