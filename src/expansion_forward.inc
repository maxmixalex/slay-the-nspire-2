static bool resolving=false;
static bool start_instance(State&,int,int,bool);
static void extension_effect(State&,Effect);
static void finish_extended_card(State&);
static void draw_hook(State&);
static void shuffle_hook(State&);
static void clear_extended_choice(State&);
static void queue_legacy(State&,Card,int,int);
static int powered_block(const State&,Card,int);
