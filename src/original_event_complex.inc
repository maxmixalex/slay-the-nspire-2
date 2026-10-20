// Additional model-specific state machines are kept separate from shared queue code.
bool complex_implemented(OriginalEventId){return false;}
bool complex_begin(State&){return false;}
bool complex_enabled(const State&,unsigned){return false;}
bool complex_choose(State&,unsigned){return false;}
bool complex_allowed(const State&,OriginalEventId,unsigned,unsigned){return false;}
bool complex_special(State&,int,int,int,int,int){return false;}
