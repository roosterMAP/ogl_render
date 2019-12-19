#pragma once
#include <vector>
#include "String.h"

class CommandSys;
extern CommandSys * g_cmdSys;

class CVar;
extern CVar * g_cvar_debugLighting;

typedef struct Cmd {
	Str name;
	Str description;
	void ( *fn )( Str args );
} Cmd;

/*
================================
CommandSys
	-Singleton class that stores pointers to all available commands.
	-When created for the first time BuildCommands() is run which creates all unique commands
	-and adds them to m_commands. Each element is a struct called Cmd which contains a function
	-pointer. The function of a Cmd is also assigned in BuildCommands().
	-When a user presses ENTER in the console, the input is validated in Console::UpdateInputLine_NewLine.
	-If its a valid input, the appropriate command is called using CallByName() which in turn calls the function
	-pointed to by Cmd::fn.
================================
*/
class CommandSys {
	public:
		static CommandSys* getInstance();
		~CommandSys() {};

		const unsigned int CommandCount() const { return m_commands.size(); }
		const Cmd* CommandByIndex( unsigned int i ) const;
		const Cmd* CommandByName( Str name ) const;
		const bool CallByName( Str name ) const;

	private:
		static CommandSys* inst_; //single instance
		CommandSys() { BuildCommands(); }
        CommandSys( const CommandSys& ); //don't implement
        CommandSys& operator=( const CommandSys& ); //don't implement

		void BuildCommands(); //creates command objects and adds them to the m_commands list

		std::vector< Cmd* > m_commands;
};

/*
================================
CVar
	-CVars extend the functionality of a command into the broader program.
	-They are global extern objects that are regularly check for their state.
	-Their states and args are manipulated by its own commands function pointer ( Cmd::fn ).
================================
*/
class CVar {
	public:
		CVar() { m_state = false; }
		~CVar() {};

		void ToggleState() { m_state = !m_state; }
		bool GetState() const { return m_state; }
		void SetState( bool state ) { m_state = state; }

		const Str& GetArgs() const { return m_args; }
		void SetArgs( Str args ) { m_args = args; }

	private:
		bool m_state;
		Str m_args;
};