
function speak(port, str)
   local wb = port:prepare()
    wb:clear()
    wb:addString(str)
    port:write()
   yarp.Time_delay(1.0)
end

----------------------------------
-- functions MOTOR - IOL        --
----------------------------------

function IOL_goHome(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("home")
    port:write(wb,reply)
end

function IOL_calibrate(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("cata")
    port:write(wb,reply)
end

function IOL_where_is(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("where")
   wb:addString(objName)
    port:write(wb,reply)
   return reply
end

function IOL_reward(port, reward)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString(reward)
    port:write(wb,reply)
end

function IOL_track_start(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("track")
   wb:addString("start")
    port:write(wb,reply)
end

function IOL_track_stop(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("track")
   wb:addString("stop")
    port:write(wb,reply)
end

function IOL_populate_name(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("name")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_take(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("take")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_grasp(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("grasp")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_touch(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("touch")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_push(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("push")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_forget(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("forget")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_explore(port, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("explore")
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_what(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("what")
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_this_is(port,objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("this")
    wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_calib_kin_start(port, side, objName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("caki")
   wb:addString("start")
   wb:addString(side)
   wb:addString(objName)
    port:write(wb,reply)
   return reply:get(0):asString()
end

function IOL_calib_kin_stop(port)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
   wb:addString("caki")
   wb:addString("stop")
   port:write(wb,reply)
   return reply:get(0):asString()
end

----------------------------------
-- functions SPEECH             --
----------------------------------

function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

function splitString(inputstr)

    local tmp = yarp.Bottle()

    for token in string.gmatch(inputstr, "[^%s]+") do
        tmp:addString(token)
    end
    return tmp
end

function Receive_Speech(port)
    local str = yarp.Bottle()
    local reply = yarp.Bottle()
    str = port:read(false)
    local inString
    if str ~= nil then
        inString = str:toString()

        if string.find(inString, "^%a") then                --check if string needs to be modified
            --do nothing string starts with character
        else
            inString = string.sub(inString, 2, -2)          --remove FIRST and LAST element of string in this case quotation marks
        end

        if string.find(inString, "^%l") then
            inString = inString:gsub("^%l", string.upper)   --put a capital letter on the first character of the string
        end

        print ("Query is: ", inString)

        reply = splitString(inString)
    end

    return reply
end
