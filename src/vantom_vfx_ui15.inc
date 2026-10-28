    // NVantomVfx's four original emitters. Birth records contain only motion
    // facts; pixel storage remains in Content. The three world emitters never
    // re-query a historical mesh while painting a particle.
    struct VantomParticle {
        uint32_t born=0;
        int32_t x=0,y=0,vx=0,vy=0; // Q12 world-projected or explosion-local units
        uint16_t lifetime=0,scale=0;
        int16_t angle=0;
        uint8_t frame=0,red=255,green=255,blue=255;
        bool active=false;
    };
    static_assert(sizeof(VantomParticle)<=32,"Vantom particles contain no pixel buffers");
    struct VantomEffects {
        const App*owner=nullptr;uint32_t seed=0,run=0,identity=0;
        int floor=-1,combat=-1,encounter=-1,clip=-1;
        uint32_t began=0,now=0,restart_at=0;unsigned restarts=0;
        uint16_t bone_scale=1024;bool loop=false;
        vantom_vfx::Clock clock{};
        VantomParticle particles[210]{}; // exactly80+50+50+30 authored slots
    };
    static VantomEffects* vantom_effects(){static VantomEffects states[MaxEnemies];return states;}
    static void vantom_vfx_reset(unsigned actor){if(actor&&actor<=MaxEnemies)reset_in_place(vantom_effects()[actor-1]);}
    static uint32_t vantom_particle_hash(uint32_t value){
        value=((value>>16)^value)*73244475u;value=((value>>16)^value)*73244475u;return(value>>16)^value;
    }
    static unsigned vantom_random(uint32_t&seed){
        // Godot4.5.1 ParticleProcessMaterial's RNG. Its independent cosmetic
        // RANDOM_SEED is deliberately unrelated to the saved combat RNG.
        int32_t value=int32_t(seed);if(!value)value=305420679;
        const int32_t quotient=value/127773;
        int64_t next=16807ll*(value-int64_t(quotient)*127773)-2836ll*quotient;
        if(next<0)next+=2147483647;seed=uint32_t(next);return seed&65535u;
    }
    static int vantom_between(int low,int high,unsigned value){return low+int(int64_t(high-low)*value/65535);}
    struct VantomBirth {
        int x=0,y=0,vx=0,vy=0,angle=0; // positions Q8, velocity in original px/s
        unsigned scale=4096,lifetime=0,frame=0,red=255,green=255,blue=255;
    };
    static VantomBirth vantom_birth(unsigned kind,uint32_t random){
        using namespace vantom_vfx;VantomBirth out;uint32_t seed=random;
        const bool explosion=kind==Explosion,spray=kind==Spray;
        out.scale=unsigned(vantom_between(spray?2458:4096,explosion?10240:6144,vantom_random(seed)));
        vantom_random(seed); // hue, authored0
        vantom_random(seed); // animation speed, authored0
        const unsigned frame=vantom_random(seed);out.frame=explosion?mn(3,int(frame*4u/65535u)):0;
        const unsigned lifetime_random=vantom_random(seed);
        out.lifetime=vantom_vfx::lifetime(Emitter(kind),lifetime_random);
        if(explosion){const unsigned tint=vantom_random(seed);
            out.red=unsigned(vantom_between(177,255,tint));out.green=unsigned(vantom_between(157,255,tint));out.blue=unsigned(vantom_between(127,255,tint));}
        const unsigned angle=vantom_random(seed);
        const int degrees=kind==DeathFront?vantom_between(25900,30900,angle):kind==DeathBack?-40000:0;
        out.angle=-degrees*4096/360000;
        vantom_random(seed); // angular velocity: ignored by align-Y; otherwise<.00002deg/s
        const int velocity=vantom_between(spray?500:explosion?3000:1000,spray?2500:explosion?5000:4000,vantom_random(seed));
        for(unsigned i=0;i<3;++i)vantom_random(seed); // directional/radial/orbit velocity
        for(unsigned i=0;i<4;++i)vantom_random(seed); // physical parameters, all authored0
        vantom_random(seed); // amount ratio, authored1
        if(explosion){
            vantom_random(seed); // ring height is on disabledZ
            const int phase=int(vantom_random(seed)*4096u/65535u);
            const int radius=int(CombatCardMotion::square_root(vantom_random(seed)*10000u));
            out.x=radius*CombatCardMotion::sine(phase)/1024;
            out.y=radius*CombatCardMotion::sine(phase+1024)/1024;
        }
        const int spread=spray?20:explosion?45:5;
        const int center=explosion?-1024:CombatCardMotion::direction(-1024,kind==DeathBack?717:-512);
        const int phase=center+vantom_between(-spread*4096,spread*4096,vantom_random(seed))/360;
        out.vx=velocity*CombatCardMotion::sine(phase+1024)/1024;
        out.vy=velocity*CombatCardMotion::sine(phase)/1024;return out;
    }
    static unsigned vantom_particle_curve(unsigned kind,bool y,unsigned age,unsigned lifetime){
        // The source Curve resources'100-sample Hermite bake is resampled at
        //33 uniformly spaced knots. XYZ anisotropy and nonzero endpoints are retained.
        static constexpr uint16_t curves[6][33]={
            {1258,1344,1490,1681,1907,2160,2429,2706,2981,3246,3490,3706,3882,4011,4083,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4077,3893,3529,3009,2360,1608,777,60},
            {1265,2679,4483,6059,6789,6683,6326,5797,5160,4481,3825,3255,2792,2422,2128,1895,1707,1549,1408,1283,1173,1075,988,910,839,773,710,649,588,524,456,383,301},
            {2322,2344,2376,2420,2477,2547,2631,2729,2844,2982,3156,3351,3552,3743,3907,4028,4090,4096,4096,4096,4096,4096,4096,4096,4096,4077,3893,3529,3009,2360,1608,777,60},
            {1265,2935,5037,6252,6160,5700,4998,4147,3412,2898,2563,2362,2236,2138,2065,2013,1980,1962,1956,1960,1970,1983,1996,2005,2007,1923,1720,1440,1122,807,535,348,286},
            {2170,2137,2110,2089,2074,2063,2056,2053,2055,2159,2390,2706,3066,3427,3746,3981,4091,4096,4096,4096,4096,4096,4096,4096,4076,3981,3795,3505,3095,2554,1866,1020,0},
            {8192,8185,8167,8142,8113,8083,8056,8035,8024,7949,7601,7039,6350,5622,4941,4393,4064,3999,3969,3919,3860,3807,3773,3762,3717,3611,3429,3158,2785,2296,1678,917,0}
        };
        const auto*curve=curves[(kind==vantom_vfx::Spray?0:kind==vantom_vfx::Explosion?4:2)+unsigned(y)];
        if(!lifetime||age>=lifetime)return curve[32];const unsigned position=age*32u,index=position/lifetime,fraction=position%lifetime;
        return (unsigned(curve[index])*(lifetime-fraction)+unsigned(curve[index+1])*fraction+lifetime/2)/lifetime;
    }
    void vantom_vfx_sync(unsigned actor,int clip,uint32_t milliseconds,bool loop,int bone_scale,uint32_t at=UINT32_MAX){
        if(!actor||actor>MaxEnemies)return;using namespace vantom_vfx;
        const uint32_t visible_now=a.animation*33u,now=at==UINT32_MAX?visible_now:at;
        auto&s=vantom_effects()[actor-1];const uint32_t identity=ui_feedback_identity(a.game,actor);
        if(s.owner!=&a||s.seed!=a.game.seed||s.run!=a.run_number||s.floor!=a.game.floor||
           s.combat!=a.game.world.combat_count||s.encounter!=int(a.game.enemy_world.encounter)||s.identity!=identity||int32_t(now-s.now)<0){
            reset_in_place(s);s.owner=&a;s.seed=a.game.seed;s.run=a.run_number;s.floor=a.game.floor;
            s.combat=a.game.world.combat_count;s.encounter=int(a.game.enemy_world.encounter);s.identity=identity;
        }
        const auto*directory=world_enemy_clip_directory(unsigned(clip));if(!directory)return;
        const uint32_t began=now-milliseconds;
        const unsigned units=unsigned(creature_sprite_units(EnemyId::Vantom));
        auto emit=[&](Emitter kind,uint32_t born,unsigned ordinal){
            const uint32_t random=vantom_particle_hash((s.seed^s.identity^uint32_t(kind)*0x9e3779b9u)+born+ordinal+1u);
            const auto birth=vantom_birth(kind,random);if(visible_now-born>=birth.lifetime)return;
            static constexpr unsigned offsets[]={0,80,130,180},amounts[]={80,50,50,30};
            auto&p=s.particles[offsets[kind]+ordinal%amounts[kind]];p={};
            p.born=born;p.lifetime=uint16_t(birth.lifetime);p.scale=uint16_t(birth.scale);p.angle=int16_t(birth.angle);
            p.frame=uint8_t(birth.frame);p.red=uint8_t(birth.red);p.green=uint8_t(birth.green);p.blue=uint8_t(birth.blue);
            if(kind==Explosion){p.x=birth.x*16;p.y=birth.y*16;p.vx=birth.vx*4096;p.vy=birth.vy*4096;}
            else{
                const bool current=int32_t(born-began)>=0||s.clip<0;
                const int source=current?clip:s.clip;const uint32_t origin=current?began:s.began;
                VantomEmitter emitter;
                const int birth_scale=vantom_motion(actor).scale.sample(born);
                if(!vantom_emitter(source,born-origin,current?loop:s.loop,unsigned(kind),birth_scale,units,emitter))return;
                p.x=emitter.x;p.y=emitter.y;
                p.vx=int32_t((int64_t(emitter.a)*birth.vx+int64_t(emitter.b)*birth.vy)/256);
                p.vy=int32_t((int64_t(emitter.c)*birth.vx+int64_t(emitter.d)*birth.vy)/256);
            }
            p.active=true;
        };
        auto restart=[&](uint32_t at){
            // The exact source callback is already included in the composite
            // die sample: keep body die while track1 charge_up_1→charged_1.
            s.restart_at=at;++s.restarts;
        };
        const auto body=vantom_vfx::animation(directory->native_clip);
        const unsigned original=directory->original_ms;
        if(body!=Idle&&body!=Die&&original&&directory->duration_ms>original&&milliseconds>=original){
            // A complete charge-up clip can retain its longer tail track
            // after the body's667ms animation has already become Idle.
            // Its idle event belongs to that original body clock.
            const uint32_t idle_at=began+original;
            if(s.clock.current!=Idle||s.clock.started!=idle_at)
                s.clock.observe(body,began,idle_at,false,emit,restart,visible_now);
            s.clock.observe(Idle,idle_at,now,true,emit,restart,visible_now);
        }else s.clock.observe(body,began,now,loop,emit,restart,visible_now);
        s.clip=clip;s.began=began;s.loop=loop;s.bone_scale=uint16_t(bone_scale);s.now=now;
    }
    void vantom_vfx_transition(unsigned actor,int old_clip,uint32_t old_started,bool old_loop,
                               int next_clip,uint32_t next_started,bool next_loop){
        // Consume every actual animator transition, including a short idle
        // that starts and finishes between two painted frames. That idle's
        // spray_off must survive a delayed render; no gameplay clock changes.
        const uint32_t at=next_started*33u;
        const int bone_scale=vantom_motion(actor).scale.sample(at);
        if(old_clip>=0&&int32_t(next_started-old_started)>=0)
            vantom_vfx_sync(actor,old_clip,(next_started-old_started)*33u,old_loop,bone_scale,at);
        vantom_vfx_sync(actor,next_clip,0,next_loop,bone_scale,at);
    }
    unsigned vantom_dissolve(unsigned actor)const{return actor&&actor<=MaxEnemies?vantom_effects()[actor-1].clock.dissolve(a.animation*33u):0;}
    void vantom_particle_image(int image,int32_t cx,int32_t cy,int32_t xx,int32_t xy,int32_t yx,int32_t yy,unsigned red,unsigned green,unsigned blue){
        poll();
        const auto p=a.content.image(image);if(!p.pixels||p.format>3)return;
        const int64_t det=int64_t(xx)*yy-int64_t(xy)*yx;if(!det)return;
        const int64_t bound_x=int64_t(CombatCardMotion::magnitude(xx))*p.w+int64_t(CombatCardMotion::magnitude(xy))*p.h;
        const int64_t bound_y=int64_t(CombatCardMotion::magnitude(yx))*p.w+int64_t(CombatCardMotion::magnitude(yy))*p.h;
        const int left=mx(0,int((int64_t(cx)*256-bound_x/2)/1048576)-1),right=mn(clip_w,int((int64_t(cx)*256+bound_x/2)/1048576)+2);
        const int top=mx(0,int((int64_t(cy)*256-bound_y/2)/1048576)-1),bottom=mn(clip_h,int((int64_t(cy)*256+bound_y/2)/1048576)+2);
        if(left>=right||top>=bottom)return;
        const int64_t ia=int64_t(yy)*4294967296ll/det,ib=-int64_t(xy)*4294967296ll/det;
        const int64_t ic=-int64_t(yx)*4294967296ll/det,id=int64_t(xx)*4294967296ll/det;
        for(int py=top;py<bottom;++py){poll_row(unsigned(py-top));for(int px=left;px<right;++px){
            unsigned coverage=0,r=0,g=0,b=0;
            // Four fixed subpixel samples retain fine9×20 spray coverage on
            // the LCD without magnifying tiny particles or a scratch bitmap.
            for(int sy=1;sy<4;sy+=2)for(int sx=1;sx<4;sx+=2){
                const int64_t dx=int64_t(px)*4096+sx*1024-cx,dy=int64_t(py)*4096+sy*1024-cy;
                const int64_t uq=dx*ia+dy*ib+int64_t(p.w)*8388608,vq=dx*ic+dy*id+int64_t(p.h)*8388608;
                if(uq<0||vq<0||uq>=int64_t(p.w)*16777216||vq>=int64_t(p.h)*16777216)continue;
                const unsigned at=unsigned(vq>>24)*p.w+unsigned(uq>>24),alpha=p.alpha?p.alpha[at]:255;
                if(!alpha)continue;const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
                const unsigned color=pixel[0]|unsigned(pixel[1])<<8;
                coverage+=alpha;r+=(color>>11)*alpha;g+=((color>>5)&63)*alpha;b+=(color&31)*alpha;
            }
            if(!coverage)continue;
            const uint16_t color=uint16_t(((r/coverage)*red/255u)<<11|((g/coverage)*green/255u)<<5|((b/coverage)*blue/255u));
            paint(py*pitch+px,color,(coverage+2u)/4u);
        }}
    }
    void vantom_draw_particles(unsigned actor,unsigned kind,int x,int y,int scale,unsigned reference_units){
        if(!actor||actor>MaxEnemies||kind>=vantom_vfx::EmitterCount||!reference_units||scale<=0)return;
        using namespace vantom_vfx;auto&s=vantom_effects()[actor-1];const unsigned now=a.animation*33u;
        const unsigned units=unsigned(creature_sprite_units(EnemyId::Vantom));
        const int projection=int(int64_t(scale)*units/reference_units);
        const bool explosion=kind==Explosion;VantomEmitter emitter;
        if(explosion&&!vantom_emitter(s.clip,now-s.began,s.loop,kind,s.bone_scale,units,emitter))return;
        static constexpr unsigned offsets[]={0,80,130,180},amounts[]={80,50,50,30};
        for(unsigned i=offsets[kind],end=i+amounts[kind];i<end;++i){auto&p=s.particles[i];if(!p.active)continue;
            const unsigned age=now-p.born;if(age>=p.lifetime){p.active=false;continue;}
            const unsigned simulated=explosion?age:age*6u/5u;
            const int gravity=explosion?8000*4096:int(1000ll*1024*4096/units);
            const int32_t px=p.x+int32_t(int64_t(p.vx)*simulated/1000),py=p.y+int32_t(int64_t(p.vy)*simulated/1000+int64_t(gravity)*simulated*simulated/2000000);
            const int vy=p.vy+int(int64_t(gravity)*simulated/1000);
            const int angle=(kind==Spray||explosion)?CombatCardMotion::direction(vy,p.vx)-1024:p.angle;
            const int sn=CombatCardMotion::sine(angle),cs=CombatCardMotion::sine(angle+1024);
            const int sx=int(unsigned(p.scale)*vantom_particle_curve(kind,false,age,p.lifetime)/4096u);
            const int sy=int(unsigned(p.scale)*vantom_particle_curve(kind,true,age,p.lifetime)/4096u);
            int32_t cx,cy,xx,xy,yx,yy;
            if(explosion){
                cx=emitter.x+int32_t((int64_t(emitter.a)*px+int64_t(emitter.b)*py)/1048576);
                cy=emitter.y+int32_t((int64_t(emitter.c)*px+int64_t(emitter.d)*py)/1048576);
                xx=int32_t((int64_t(emitter.a)*cs+int64_t(emitter.b)*sn)*sx/4194304);
                xy=int32_t((-int64_t(emitter.a)*sn+int64_t(emitter.b)*cs)*sy/4194304);
                yx=int32_t((int64_t(emitter.c)*cs+int64_t(emitter.d)*sn)*sx/4194304);
                yy=int32_t((-int64_t(emitter.c)*sn+int64_t(emitter.d)*cs)*sy/4194304);
            }else{
                cx=px;cy=py;xx=int32_t(int64_t(cs)*sx*256/units);xy=int32_t(-int64_t(sn)*sy*256/units);
                yx=int32_t(int64_t(sn)*sx*256/units);yy=int32_t(int64_t(cs)*sy*256/units);
            }
            cx=x*4096+int32_t(int64_t(cx)*projection/1024);cy=y*4096+int32_t(int64_t(cy)*projection/1024);
            xx=int32_t(int64_t(xx)*projection/1024);xy=int32_t(int64_t(xy)*projection/1024);
            yx=int32_t(int64_t(yx)*projection/1024);yy=int32_t(int64_t(yy)*projection/1024);
            vantom_particle_image(a.content.ui_image(explosion?DeathParticleStart+p.frame:BasicParticle),cx,cy,xx,xy,yx,yy,p.red,p.green,p.blue);
        }
    }
