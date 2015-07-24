
event_table = {
   See       = "e_exit",
   Calibrate = "e_calibrate",
   Where     = "e_where",
   I         = "e_i_will",
   Take      = "e_take",
   Grasp     = "e_grasp",
   Return    = "e_home",
   Touch     = "e_touch",
   Push      = "e_push",
   Forget    = "e_forget",
   Explore   = "e_explore",
   What      = "e_what",
   Let       = "e_let",
   }


interact_fsm = rfsm.state{

    ----------------------------------
    -- state SUB_MENU               --
    ----------------------------------
    SUB_MENU = rfsm.state{
        entry=function()
            print("in substate MENU : waiting for speech command!")
        end,

            doo = function()
            while true do
            
                result = Receive_Speech(speechRecog_port)
                local cmd
                if result ~= nil then
                    --print("received REPLY: ", result:toString() )
                    cmd = result:get(0):asString()
                else    
                    cmd =  "waiting" -- do nothing
                end
            
                rfsm.send_events(fsm, event_table[cmd])
                rfsm.yield(true)
            end
        end
    },

    ----------------------------------
    -- state SUB_EXIT               --
    ----------------------------------

    SUB_EXIT = rfsm.state{
        entry=function()
            speak(ispeak_port, "Ok, bye bye")
            rfsm.send_events(fsm, 'e_menu_done')
        end
    },
   
    ----------------------------------
    -- state SUB_CALIBRATE          --
    ----------------------------------

    SUB_CALIBRATE = rfsm.state{
        entry=function()
            IOL_calibrate(iol_port)
            speak(ispeak_port,"OK, I know the table height")
    end
    },
   
    ----------------------------------
    -- state SUB_WHERE              --
    ----------------------------------

    SUB_WHERE = rfsm.state{
        entry=function()
            local obj = result:get(3):asString()
            local b = IOL_where_is(iol_port, obj)
            local ret = b:get(0):asString()
            
            if  ret == "ack" or ret == "nack" then
        
                print("SUB_WHERE : waiting for speech command!")
                
                repeat
                    reward = Receive_Speech(speechRecog_port)
                until reward:size() > 0
                print("reward received REPLY: ", reward:toString() )
    
                local cmd  =  reward:get(0):asString()
                
                print("REWARD IS", cmd)
                if cmd == "Yes" then
                    IOL_reward(iol_port,"ack")
                elseif cmd == "No" then
                    print("in no")
                    IOL_reward(iol_port,"nack")
                elseif cmd == "Skip" then
                    IOL_reward(iol_port,"skip")
                else
                    IOL_reward(iol_port,"skip")
                    speak(ispeak_port,"I don't understand")
                end
            else
                print("I dont get it")
                speak(ispeak_port,"something is wrong")
            end
        end
    },
   
    ----------------------------------
    -- state SUB_TEACH_OBJ          --
    ----------------------------------

    SUB_TEACH_OBJ = rfsm.state{
        entry=function()
            
            IOL_track_start(iol_port)
            
            print("SUB_TEACH : waiting for speech command!")
        
            local reward
            repeat
                reward = Receive_Speech(speechRecog_port)
            until reward:size() > 0
            
            print("reward received REPLY: ", reward:toString() )
        
            local cmd  =  reward:get(0):asString()
            
            print("REPLY IS", cmd)
            if cmd == "There" then
                IOL_track_stop(iol_port)
                print("stopped track")
                
                print("SUB_TEACH_OBJ_NAME : waiting for speech command!")
                
                speak(ispeak_port,"I will learn the new object, please say its name");
                
                local str
                repeat
                    str = Receive_Speech(speechRecog_port)
                until str:size() > 0
                
                local objName  =  str:get(0):asString()
                
                if objName == "ERROR" then
                    speak(ispeak_port,"Skipped");
                else
                    print("done with name ", objName)
                    local ret = IOL_populate_name(iol_port, objName)
                    print("REPLY IS", ret)
                end
            elseif cmd == "Skip" then
                IOL_track_stop(iol_port)
            else
                IOL_track_stop(iol_port)
                speak(ispeak_port,"I don't understand")
            end
        end
    },

    ----------------------------------
    -- state SUB_TAKE               --
    ----------------------------------

    SUB_TAKE = rfsm.state{
        entry=function()
            local obj = result:get(2):asString()
            local b = IOL_take(iol_port, obj)
        end
    },
   
    ----------------------------------
    -- state SUB_GRASP              --
    ----------------------------------
    
    SUB_GRASP = rfsm.state{
        entry=function()
            local obj = result:get(2):asString()
            local b = IOL_grasp(iol_port, obj)
        end
    },
   
    ----------------------------------
    -- state SUB_RETURN             --
    ----------------------------------
    
    SUB_RETURN = rfsm.state{
        entry=function()
            print("going home!")
            IOL_goHome(iol_port)
        end
    },
    
    ----------------------------------
    -- state SUB_TOUCH              --
    ----------------------------------
    
    SUB_TOUCH = rfsm.state{
        entry=function()
            local obj = result:get(2):asString()
            local b = IOL_touch(iol_port, obj)
        end
    },
   
    ----------------------------------
    -- state SUB_PUSH              --
    ----------------------------------
    
    SUB_PUSH = rfsm.state{
        entry=function()
            local obj = result:get(2):asString()
            local b = IOL_push(iol_port, obj)
        end
    },
    
    ----------------------------------
    -- state SUB_FORGET             --
    ----------------------------------
    
    SUB_FORGET = rfsm.state{
        entry=function()
            local obj 
            obj= result:get(1):asString()
            if obj == "the" then
                obj= result:get(2):asString()
            end
            local b = IOL_forget(iol_port, obj)
        end
    },
    
    ----------------------------------
    -- state SUB_EXPLORE            --
    ----------------------------------   
    
    SUB_EXPLORE = rfsm.state{
        entry=function()
            local obj = result:get(2):asString()
            local b = IOL_explore(iol_port, obj)
        end
    },
 
    ----------------------------------
    -- state SUB_WHAT               --
    ----------------------------------  
    
    SUB_WHAT = rfsm.state{
    entry=function()
        local answer = IOL_what(iol_port)
    
            if  answer == "ack" then
    
                print("SUB_WHAT : waiting for speech command!")
    
                repeat
                    reward = Receive_Speech(speechRecog_port)
                until reward:size() > 0
    
                print("What received REPLY: ", reward:toString() )
                local cmd  =  reward:get(0):asString()
                local obj  =  reward:get(4):asString() 
                print("REWARD IS", cmd)
                if cmd == "Yes" then
                    print("YES", obj)
                    IOL_reward(iol_port,"ack")
                elseif cmd == "No" then
                    IOL_reward(iol_port,"nack")
                elseif cmd == "Skip" then
                    IOL_reward(iol_port,"skip")
                elseif cmd == "Wrong" then
                    print("WRONG WITH OBJ", obj)
                local ret = IOL_populate_name(iol_port, obj)
                    print("REPLY IS", ret)
                else
                    IOL_reward(iol_port,"skip")
                    speak(ispeak_port,"I don't understand")
                end
            elseif answer == "nack" then
                
                print("SUB_WHAT : waiting for speech command!")
                
                repeat
                    reward = Receive_Speech(speechRecog_port)
                until reward:size() > 0
                
                local cmd  =  reward:get(0):asString()
                local obj  =  reward:get(3):asString()
                if cmd == "Skip" then
                    IOL_reward(iol_port,"skip")
                else
                    local ret = IOL_populate_name(iol_port, obj)
                    print("REPLY IS", ret)
                end
            else
                print("I dont get it")
                speak(ispeak_port,"something is wrong")
            end
        end
    },
   
    ----------------------------------
    -- state SUB_LET                --
    ----------------------------------    
    
    SUB_LET = rfsm.state{
    entry=function()
        let_obj = result:get(8):asString()
        let_arm = result:get(11):asString()
        speak(ispeak_port,"Do you mean show me how to reach the " .. let_obj .. " with my " .. let_arm .." arm? ")
        
        print("SUB_LET : waiting for speech command!")
        repeat
            reward = Receive_Speech(speechRecog_port)
        until reward:size() > 0

        let_cmd  =  reward:get(0):asString()
    end,
    
    doo = function()
        while let_cmd == "Yes" do
            print("Enter teaching mode")
            print("arm is ", let_arm)
            print("Obj is ", let_obj)
            local ret = IOL_calib_kin_start(iol_port, let_arm, let_obj)
            if  ret == "ack" then
                rfsm.send_events(fsm, "e_kin")
            else
                local fin
                print("SUB_LET : waiting for speech command!")
                repeat
                    fin = Receive_Speech(speechRecog_port)
                until reward:size() > 0
                
                local cmd  =  fin:get(0):asString()
                if cmd == "No" then
                    let_cmd = "done"
                elseif cmd ~= "Yes" then
                    speak(ispeak_port, "Sorry I do not understand")
                end
            end
                rfsm.yield(true)
        end
    end
    },

    ----------------------------------
    -- state SUB_KIN                --
    ---------------------------------- 
    
    SUB_KIN = rfsm.state{
        doo = function()
            local finish = false
            while not finish do
            
                print("SUB_KIN : waiting for speech command!")
                local fin
                repeat
                    fin = Receive_Speech(speechRecog_port)
                until reward:size() > 0
            
                local cmd  =  fin:get(0):asString()
                if cmd == "Finished" then
                    IOL_calib_kin_stop(iol_port)
                    finish = true
                end
                rfsm.yield(true)
            end
        end
    },

    ----------------------------------
    -- state transitions            --
    ----------------------------------
    
    rfsm.trans{ src='initial', tgt='SUB_MENU'},
    rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_CALIBRATE', events={ 'e_calibrate' } },
    rfsm.transition { src='SUB_CALIBRATE', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_WHERE', events={ 'e_where' } },
    rfsm.transition { src='SUB_WHERE', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_TEACH_OBJ', events={ 'e_i_will' } },
    rfsm.transition { src='SUB_TEACH_OBJ', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_TAKE', events={ 'e_take' } },
    rfsm.transition { src='SUB_TAKE', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_GRASP', events={ 'e_grasp' } },
    rfsm.transition { src='SUB_GRASP', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_RETURN', events={ 'e_home' } },
    rfsm.transition { src='SUB_RETURN', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_TOUCH', events={ 'e_touch' } },
    rfsm.transition { src='SUB_TOUCH', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_PUSH', events={ 'e_push' } },
    rfsm.transition { src='SUB_PUSH', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_FORGET', events={ 'e_forget' } },
    rfsm.transition { src='SUB_FORGET', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_EXPLORE', events={ 'e_explore' } },
    rfsm.transition { src='SUB_EXPLORE', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_WHAT', events={ 'e_what' } },
    rfsm.transition { src='SUB_WHAT', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_MENU', tgt='SUB_LET', events={ 'e_let' } },
    rfsm.transition { src='SUB_LET', tgt='SUB_MENU', events={ 'e_done' } },
    
    rfsm.transition { src='SUB_LET', tgt='SUB_KIN', events={ 'e_kin' } },
    rfsm.transition { src='SUB_KIN', tgt='SUB_MENU', events={ 'e_done' } },

}
