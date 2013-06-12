
event_table = {
	See			= "e_exit",
	Calibrate   = "e_calibrate",
	Where	    = "e_where",
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
				speak(ispeak_port, "What should I do?")
				result = SM_Reco_Grammar(speechRecog_port, grammar)
				print("received REPLY: ", result:toString() )
				local cmd =  result:get(1):asString():c_str()
				rfsm.send_events(fsm, event_table[cmd])
                rfsm.yield(true)
            end
		end
	},

	----------------------------------
	-- states                       --
	----------------------------------

	SUB_EXIT = rfsm.state{
		entry=function()
			speak(ispeak_port, "Ok, bye bye")
			rfsm.send_events(fsm, 'e_menu_done')
		end
	},

	SUB_CALIBRATE = rfsm.state{
		entry=function()
			IOL_calibrate(iol_port)
			speak(ispeak_port,"OK, I know the table height")
		end
	},

	SUB_WHERE = rfsm.state{
		entry=function()
			local obj = result:get(7):asString():c_str()
			local b = IOL_where_is(iol_port, obj)
			local ret = ""
			--if b ~= nil then
				ret = b:get(0):asString():c_str()
			--end
			print("RESPOND")
			if  ret == "ack" or ret == "nack" then

				local reward = SM_Reco_Grammar(speechRecog_port, grammar_reward)
				print("received REPLY: ", reward:toString() )
				local cmd  =  reward:get(1):asString():c_str()
				print("REWARD IS", cmd)
				if cmd == "Yes" then
					IOL_reward(iol_port,"ack")
				elseif cmd == "No" then
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
	-- state transitions            --
	----------------------------------

	rfsm.trans{ src='initial', tgt='SUB_MENU'},
	rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_CALIBRATE', events={ 'e_calibrate' } },
	rfsm.transition { src='SUB_CALIBRATE', tgt='SUB_MENU', events={ 'e_done' } },

	rfsm.transition { src='SUB_MENU', tgt='SUB_WHERE', events={ 'e_where' } },
	rfsm.transition { src='SUB_WHERE', tgt='SUB_MENU', events={ 'e_done' } },


}
