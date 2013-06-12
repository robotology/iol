
event_table = {
	See			= "e_exit",
	Calibrate   = "e_calibrate",
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
				print("received REPLY: ")
				print(result:toString())
				cmd =  result:get(1):asString():c_str()

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

	--SUB_CALIBRATE = rfsm.state{
		--entry=function()
			--rfsm.send_events(fsm, 'e_menu_done')
			--speak(ispeak_port,"OK, I know the table height")
		--end
	--},

	----------------------------------
	-- state transitions            --
	----------------------------------

	rfsm.trans{ src='initial', tgt='SUB_MENU'},
	rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

}
