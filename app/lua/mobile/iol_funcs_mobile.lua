function IOL_Mobile_Initialize()
  -- initilization
  ispeak_port = yarp.BufferedPortBottle()
  speechRecog_port = yarp.BufferedPortBottle()
  iol_port = yarp.Port()
  object_port = yarp.Port()

  -- defining objects and actions vocabularies
  objects = {"Octopus", "Lego", "Toy", "Ladybug", "Turtle", "Car", "Bottle", "Box"}
  actions = {"{point at}", "{what is this}"}

  -- defining speech grammar for Menu
  grammar = "Return to home position | Calibrate on table | Where is the #Object | Take the #Object | Grasp the #Object | See you soon  | I will teach you a new object | "
         .."Touch the #Object | Push the #Object | Let me show you how to reach the #Object with your right arm | Let me show you how to reach the #Object with your left arm | "
          .."Forget #Object | Forget all objects | Execute a plan | What is this | This is a #Object | Explore the #Object "

  -- defining speech grammar for Reward
  grammar_reward = "Yes you are | No here it is | Skip it"

  -- defining speech grammar for teaching a new object
  grammar_track = "There you go | Skip it"

  -- defining speech grammar for what function (ack)
  grammar_whatAck = "Yes you are | No you are not | Skip it | Wrong this is a #Object"

  -- defining speech grammar for what function (nack)
  grammar_whatNack = "This is a #Object | Skip it"

  -- defining speech grammar teach reach
  grammar_teach = "Yes I do | No I do not | Finished"

  return (ispeak_port ~= nil) and (speechRecog_port ~= nil) and (iol_port ~= nil) and (object_port ~= nil)
end

function speak(port, str)
   local wb = port:prepare()
    wb:clear()
    wb:addString(str)
    port:write()
   yarp.delay(1.0)
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

    if str == nill then
        --print ("null ")
    else

        print ("Initial is: ", str:toString())
        print ("size is: ", str:get(0):asList():size())

        reply:clear()
        for i=0,str:get(0):asList():size()-1,1
        do
            inString = str:get(0):asList():get(i):asList():get(0):asString()
            print ("initial word is : ", inString)
            if (inString == "verb") then
                verb = str:get(0):asList():get(i):asList():get(1):asString()
                reply:addString(verb)
            end

            if (inString == "object") then
                verb = str:get(0):asList():get(i):asList():get(1):asString()
                reply:addString(verb)
            end

        end

        print ("reply is: ", reply:toString())

    end

    return reply
end





--function Receive_Speech(port)
   -- local str = yarp.Bottle()
   -- local reply = yarp.Bottle()
    --str = port:read(false)
   -- local inString
    --if str ~= nil then
        --inString = str:toString()
        --if string.find(inString, "^%a") then                --check if string needs to be modified
            --do nothing string starts with character
        --else
        --     inString = string.sub(inString, 2, -2)          --remove FIRST and LAST element of string in this case quotation marks
        -- end

        --if string.find(inString, "^%l") then
        --   inString = inString:gsub("^%l", string.upper)   --put a capital letter on the first character of the string
        --end

       -- print ("Query is: ", inString)

     --   reply = splitString(inString)
   -- end

    --return reply
    --end
