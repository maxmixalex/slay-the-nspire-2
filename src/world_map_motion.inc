// Godot 4.5.1 easing equations, evaluated into bounded integer samples.
// EXPO_OUT uses the authored 1.001 factor. The marker's 0.3-period
// ELASTIC_OUT keeps its overshoot; it is not a monotone bounce replacement.
// Tables have 256 (EXPO/ELASTIC) or 128 (SINE) intervals with linear
// interpolation. Q16 weights/Q12 source positions are a pixel-quantization
// adaptation, not a claim of GPU or binary32 tween equality.
static const uint32_t map_motion_expo[]={
    0,1752,3458,5118,6734,8306,9837,11326,12776,14187,15561,16897,
    18198,19465,20697,21897,23064,24200,25306,26383,27430,28450,29443,30408,
    31349,32264,33154,34021,34864,35686,36485,37262,38019,38756,39473,40171,
    40851,41512,42155,42782,43391,43985,44562,45124,45671,46203,46722,47226,
    47717,48195,48660,49112,49553,49981,50399,50805,51200,51585,51959,52323,
    52678,53023,53359,53686,54005,54314,54616,54909,55195,55473,55744,56007,
    56263,56513,56755,56992,57222,57446,57663,57876,58082,58283,58478,58669,
    58854,59034,59209,59380,59546,59708,59866,60019,60168,60313,60454,60592,
    60726,60856,60983,61106,61226,61343,61457,61568,61675,61780,61882,61982,
    62078,62172,62264,62353,62440,62524,62607,62687,62764,62840,62914,62986,
    63056,63124,63190,63254,63317,63378,63437,63495,63551,63606,63660,63711,
    63762,63811,63859,63905,63951,63995,64038,64080,64120,64160,64198,64236,
    64272,64308,64342,64376,64409,64441,64472,64502,64531,64560,64588,64615,
    64641,64667,64692,64716,64740,64763,64785,64807,64828,64849,64869,64888,
    64907,64926,64944,64962,64979,64995,65012,65027,65043,65058,65072,65086,
    65100,65113,65126,65139,65151,65164,65175,65187,65198,65208,65219,65229,
    65239,65249,65258,65267,65276,65285,65293,65302,65310,65318,65325,65332,
    65340,65347,65353,65360,65367,65373,65379,65385,65391,65396,65402,65407,
    65412,65417,65422,65427,65432,65436,65441,65445,65449,65453,65457,65461,
    65465,65468,65472,65475,65479,65482,65485,65488,65491,65494,65497,65500,
    65503,65505,65508,65510,65513,65515,65518,65520,65522,65524,65526,65528,
    65530,65532,65534,65536,65536,
};
static const int32_t map_motion_sine_in[]={
    0,8,31,69,123,193,277,378,493,624,770,932,
    1108,1300,1508,1730,1968,2220,2488,2771,3069,3382,3709,4052,
    4409,4781,5168,5570,5986,6416,6862,7321,7795,8283,8785,9301,
    9831,10376,10934,11506,12091,12690,13303,13929,14569,15221,15887,16566,
    17258,17962,18679,19409,20152,20906,21673,22453,23244,24047,24862,25688,
    26527,27376,28237,29109,29992,30886,31791,32706,33632,34569,35515,36472,
    37438,38414,39400,40396,41400,42414,43437,44469,45510,46559,47616,48682,
    49756,50838,51927,53024,54129,55241,56360,57486,58618,59758,60903,62055,
    63213,64377,65547,66722,67902,69088,70279,71475,72675,73880,75089,76302,
    77519,78740,79964,81192,82423,83657,84894,86133,87375,88619,89865,91113,
    92363,93614,94867,96121,97375,98631,99887,101143,102400,
};
static const int32_t map_motion_elastic_y[]={
    -102400,-99331,-95707,-91582,-87013,-82056,-76767,-71203,-65418,-59465,-53396,-47261,
    -41108,-34983,-28928,-22983,-17185,-11569,-6165,-1002,3897,8509,12817,16805,
    20461,23777,26746,29365,31635,33556,35133,36373,37286,37881,38170,38169,
    37892,37356,36579,35578,34372,32982,31427,29727,27902,25971,23955,21872,
    19740,17579,15404,13232,11080,8961,6889,4876,2934,1074,-697,-2369,
    -3935,-5391,-6731,-7952,-9051,-10027,-10878,-11607,-12213,-12699,-13067,-13322,
    -13467,-13507,-13447,-13294,-13052,-12729,-12331,-11865,-11338,-10756,-10128,-9459,
    -8758,-8030,-7282,-6522,-5754,-4985,-4221,-3466,-2726,-2006,-1309,-639,
    0,605,1174,1705,2195,2644,3050,3412,3731,4006,4237,4426,
    4573,4679,4745,4774,4767,4726,4652,4549,4419,4264,4085,3887,
    3671,3440,3197,2943,2681,2414,2143,1871,1600,1332,1068,810,
    560,319,89,-130,-336,-529,-709,-873,-1023,-1157,-1276,-1379,
    -1467,-1540,-1598,-1641,-1670,-1685,-1688,-1678,-1657,-1624,-1582,-1530,
    -1471,-1403,-1329,-1250,-1165,-1077,-985,-891,-796,-700,-604,-509,
    -415,-323,-233,-147,-64,15,90,160,226,286,341,391,
    435,474,507,535,557,575,587,594,597,595,589,579,
    566,549,529,506,481,453,424,393,361,328,295,261,
    227,193,160,127,95,64,34,6,-22,-47,-71,-93,
    -113,-131,-148,-162,-175,-185,-194,-201,-206,-209,-211,-211,
    -209,-206,-202,-197,-190,-182,-174,-164,-154,-143,-132,-121,
    -109,-97,-85,-73,-61,-50,-38,-27,-16,-6,4,13,
    22,30,37,44,0,
};
static int map_motion_round(int value,unsigned denominator){
    return value<0?-int((unsigned(-value)+denominator/2)/denominator):int((unsigned(value)+denominator/2)/denominator);
}
template<class T> static int map_motion_sample(const T*table,unsigned intervals,uint32_t elapsed,unsigned duration){
    if(elapsed>=duration)return int(table[intervals]);
    const unsigned scaled=unsigned(elapsed)*intervals,index=scaled/duration,remainder=scaled%duration;
    return int(table[index])+map_motion_round((int(table[index+1])-int(table[index]))*int(remainder),duration);
}
static unsigned map_motion_sqrt(unsigned value){
    unsigned bit=1u<<30,result=0;while(bit>value)bit>>=2;
    while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;
}
unsigned world_map_path_dot_count(const State&s,unsigned from,unsigned to){
    const auto*m=world_current_map(s);WorldMapLayout layout;
    if(!m||from>=m->count||to>=m->count||!m->points[from].children.contains(uint8_t(to))||!world_map_layout(s,layout))return 0;
    const int dx=layout.points[to].x-layout.points[from].x,dy=layout.points[to].y-layout.points[from].y;
    return map_motion_sqrt(unsigned(dx*dx+dy*dy))*100/484;
}
static uint32_t map_motion_shape(const reference::ReferenceMap&m){
    uint32_t hash=2166136261u;
    auto add=[&](unsigned value){hash^=value;hash*=16777619u;};
    add(m.count);add(m.root);add(m.boss);add(m.second_boss);add(m.config.variant);
    for(unsigned i=0;i<m.count;++i){const auto&p=m.points[i];
        add(p.active);add(p.col);add(p.row);add(unsigned(p.type));add(p.children.used);
        for(unsigned n=0;n<p.children.used&&n<reference::Columns;++n)add(p.children.slots[n]);
    }
    return hash;
}
void MapPresentationState::reset(){*this=MapPresentationState{};}
void MapPresentationState::bind(const State&s,uint32_t now){
    (void)now;const auto*m=world_current_map(s);
    if(!m||m->count>reference::MaxPoints){if(act!=255)reset();return;}
    const uint32_t current_shape=map_motion_shape(*m);
    if(act!=s.world.act_index||seed!=s.world.run_seed||shape!=current_shape){
        reset();act=s.world.act_index;seed=s.world.run_seed;shape=current_shape;
    }
    for(unsigned i=0;i<m->count;++i){auto&p=points[i];
        const bool selected=pending&&travel_to==i;
        if(!s.world.visited[act][i]&&!selected){p=MapPointPresentation{};continue;}
        const auto type=m->points[i].type;
        if(m->points[i].active&&type!=reference::PointType::Start&&type!=reference::PointType::Boss&&!p.circle_mode)p.circle_mode=2;
    }
}
void MapPresentationState::finish_marker(){
    if(marker_mode==2){marker_mode=0;marker_point=reference::NoPoint;}
    else if(marker_mode==3)marker_mode=1;
}
void MapPresentationState::set_marker(unsigned point,uint32_t now){
    finish_marker();
    // NMapMarker.SetMapPoint only positions/reveals a currently hidden child.
    if(!marker_mode&&point<reference::MaxPoints){marker_point=uint8_t(point);marker_start=now;marker_mode=3;}
}
void MapPresentationState::initial_marker(unsigned point,uint32_t now){set_marker(point,now);}
void MapPresentationState::set_open(const State&s,bool visible,uint32_t now){
    update(s,now);if(visible==map_open)return;map_open=visible;if(!visible)return;
    const auto*m=world_current_map(s);const unsigned current=s.world.current_point;
    if(!m||current>=m->count||m->root>=m->count||m->boss>=m->count)return;
    const auto row=m->points[current].row;
    // This is NMapScreen.Open's condition. Initial Neow marker placement is
    // a separate explicit EnterAct hook, never inferred from restored State.
    // Open has no IsTraveling exception: a real close/reopen can reveal the
    // old current point during its hide. The later visible guard is retained.
    if(row!=m->points[m->root].row&&row!=m->points[m->boss].row)set_marker(current,now);
}
void MapPresentationState::begin(const State&s,unsigned from,unsigned to,uint32_t now,unsigned path_ms){
    bind(s,now);const auto*m=world_current_map(s);
    if(pending||!m||to>=m->count||!m->points[to].active||s.world.visited[s.world.act_index][to]||path_ms>1000)return;
    pending=true;travel_start=now;travel_from=from<m->count?uint8_t(from):reference::NoPoint;travel_to=uint8_t(to);
    const bool edge=from<m->count&&m->points[from].children.contains(uint8_t(to));
    travel_path_ms=uint16_t(edge?path_ms:0);marker_waiting=true;
    auto&p=points[to];const auto type=m->points[to].type;
    if(type!=reference::PointType::Start&&type!=reference::PointType::Boss&&!p.circle_mode){p.circle_start=now;p.circle_mode=1;}
    if(edge){p.path_from=uint8_t(from);p.path_start=now;p.path_ms=travel_path_ms;}
    finish_marker();if(marker_mode){marker_start=now;marker_mode=2;}
    if(!travel_path_ms){set_marker(to,now);marker_waiting=false;}
}
void MapPresentationState::update(const State&s,uint32_t now){
    bind(s,now);
    for(auto&p:points){
        if(p.circle_mode==1&&uint32_t(now-p.circle_start)>=1000)p.circle_mode=2;
        if(p.path_ms&&uint32_t(now-p.path_start)>=unsigned(p.path_ms)+400)p.path_ms=0;
    }
    if(marker_mode==2&&uint32_t(now-marker_start)>=200)finish_marker();
    else if(marker_mode==3&&uint32_t(now-marker_start)>=950)finish_marker();
    if(pending&&marker_waiting&&uint32_t(now-travel_start)>=travel_path_ms){
        set_marker(travel_to,travel_start+travel_path_ms);marker_waiting=false;
        if(marker_mode==3&&uint32_t(now-marker_start)>=950)finish_marker();
    }
}
void MapPresentationState::committed(const State&s,uint32_t now){
    update(s,now);pending=false;marker_waiting=false;travel_from=travel_to=reference::NoPoint;
}
void MapPresentationState::cancel(const State&s,uint32_t now){
    const bool was_open=map_open;reset();bind(s,now);set_open(s,was_open,now);
}
bool MapPresentationState::traveled(const State&s,unsigned point)const{
    return point<reference::MaxPoints&&s.world.act_index<WorldActCount&&
        (s.world.visited[s.world.act_index][point]||(pending&&point==travel_to));
}
MapCircleSample MapPresentationState::circle(unsigned point,uint32_t now)const{
    MapCircleSample result;if(point>=reference::MaxPoints)return result;
    const auto&p=points[point];if(!p.circle_mode)return result;
    if(p.circle_mode!=1){result.alpha=242;return result;}
    const uint32_t elapsed=now-p.circle_start;
    // Nominal 24Hz authored sprite intervals resampled onto the host clock.
    // The original coroutine's individual SceneTreeTimer wakeups are also
    // frame-quantized; no universal original-PC frame timestamps are implied.
    result.frame=elapsed>=167?4:unsigned(elapsed)*24/1000;
    const unsigned alpha=unsigned(map_motion_sample(map_motion_expo,256,elapsed,500));
    const unsigned scale=unsigned(map_motion_sample(map_motion_expo,256,elapsed,1000));
    result.alpha=(alpha*969u+131072u)/262144u;
    result.scale=8192u-((scale*4096u+32768u)>>16);return result;
}
MapPathDotSample MapPresentationState::path_dot(const State&s,unsigned from,unsigned to,unsigned index,unsigned count,uint32_t now)const{
    MapPathDotSample result;
    if(from>=reference::MaxPoints||to>=reference::MaxPoints||s.world.act_index>=WorldActCount)return result;
    result.traveled=s.world.visited[s.world.act_index][from]&&s.world.visited[s.world.act_index][to];
    if(result.traveled)result.scale=4915;
    const auto&p=points[to];if(p.path_from!=from||!count||index>=count)return result;
    if(!p.path_ms){result.traveled=result.fresh=true;result.scale=4915;return result;}
    const uint32_t elapsed=now-p.path_start;
    const unsigned deadline=unsigned((uint64_t(index+1)*p.path_ms+count-1)/count);
    if(elapsed<deadline){result.traveled=false;result.scale=4096;return result;}
    result.traveled=result.fresh=true;const uint32_t age=elapsed-deadline;
    if(age>=400)result.scale=4915;
    else{const unsigned remaining=400-age;
        result.scale=4915+unsigned((uint64_t(2048)*remaining*remaining*remaining+32000000u)/64000000u);}
    return result;
}
MapMarkerSample MapPresentationState::marker(uint32_t now)const{
    MapMarkerSample result;if(!marker_mode||marker_point>=reference::MaxPoints)return result;
    result.point=marker_point;const uint32_t elapsed=now-marker_start;
    if(marker_mode==2){
        if(elapsed>=200){result.point=reference::NoPoint;return result;}
        result.scale_x=result.scale_y=(200-elapsed)*4096/200;
    }else if(marker_mode==3){
        if(elapsed<200){result.scale_x=unsigned(elapsed)*4096/200;
            result.source_y=-map_motion_sample(map_motion_sine_in,128,elapsed,200);}
        else result.source_y=map_motion_sample(map_motion_elastic_y,256,elapsed-200,750);
    }
    return result;
}
