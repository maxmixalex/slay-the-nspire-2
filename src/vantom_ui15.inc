    // Canvas members: parameter data is copied before image preparation can
    // recycle its cache page. All pixel storage remains in Content's arena.
    static unsigned vantom_word(const uint8_t*data,unsigned index){return data[2*index]|unsigned(data[2*index+1])<<8;}
    static int vantom_round(int64_t value,int denominator){return value<0?-int((-value+denominator/2)/denominator):int((value+denominator/2)/denominator);}
    using VantomEmitter=vantom_vfx::EmitterTransform;
    struct VantomMotion {const App*owner=nullptr;uint32_t seed=0,run=0,identity=0,last=0;
        int floor=-1,combat=-1,encounter=-1;CreatureScaleMotion scale{};};
    static VantomMotion* vantom_motions(){static VantomMotion motions[MaxEnemies];return motions;}
    static void vantom_reset(unsigned actor){if(actor&&actor<=MaxEnemies){vantom_motions()[actor-1]={};vantom_vfx_reset(actor);}}
    VantomMotion& vantom_motion(unsigned actor){
        auto&m=vantom_motions()[actor-1];const uint32_t now=a.animation*33u;
        const uint32_t identity=ui_feedback_identity(a.game,actor);
        const int target=a.combat_status_amount(int(actor),EnemyStatusStart+int(EnemyPower::Shrink))||
            a.combat_status_amount(int(actor),41)?512:1024;
        if(m.owner!=&a||m.seed!=a.game.seed||m.run!=a.run_number||m.floor!=a.game.floor||
            m.combat!=a.game.world.combat_count||m.encounter!=int(a.game.enemy_world.encounter)||
            m.identity!=identity||int32_t(now-m.last)<0){
            m={};m.owner=&a;m.seed=a.game.seed;m.run=a.run_number;m.floor=a.game.floor;
            m.combat=a.game.world.combat_count;m.encounter=int(a.game.enemy_world.encounter);
            m.identity=identity;m.scale.from=m.scale.to=target;m.scale.started=now;
        }
        if(target!=m.scale.to){
            // Vantom.ScaleTo completes the old tween before starting its
            //750ms SineInOut tween on Visuals/ScalingBone.
            m.scale.from=m.scale.to;m.scale.to=target;m.scale.started=now;m.scale.duration=750;
        }
        m.last=now;return m;
    }
    int vantom_scale(unsigned actor){return !actor||actor>MaxEnemies?1024:vantom_motion(actor).scale.sample(a.animation*33u);}
    struct VantomTable {const uint8_t*data=nullptr;unsigned words=0,at=0,units=0;int row=-1;};
    bool vantom_table(int clip_id,uint32_t milliseconds,bool loop,VantomTable&out){
        const int row=vantom_ui15_row(clip_id);if(row<0)return false;
        const int table_id=a.content.ui_image(VantomUi15BaseAuxiliary+unsigned(row&127));if(table_id<0)return false;
        const auto table=a.content.image(table_id);const unsigned available=unsigned(table.w)*table.h;
        if(!table.pixels||table.format!=0||table.alpha||table.palette||available<VantomUi15HeaderWords)return false;
        const auto*data=table.pixels;
        if(vantom_word(data,0)!=VantomUi15Magic||vantom_word(data,1)!=3||
           vantom_word(data,8)!=VantomUi15LayerWords||vantom_word(data,9)!=VantomUi15HeaderWords)return false;
        const unsigned frames=vantom_word(data,2),frame_ms=vantom_word(data,3),words=vantom_word(data,10);
        if(!frames||!frame_ms||words>available||words<VantomUi15HeaderWords+frames*VantomUi15FrameWords)return false;
        unsigned frame=milliseconds/frame_ms;frame=loop?frame%frames:unsigned(mn(int(frame),int(frames)-1));
        const unsigned units=vantom_word(data,4)|(vantom_word(data,5)<<16);if(!units)return false;
        out={data,words,VantomUi15HeaderWords+frame*VantomUi15FrameWords,units,row};return true;
    }
    bool vantom_emitter(int clip_id,uint32_t milliseconds,bool loop,unsigned index,int bone_scale,
                        unsigned reference_units,VantomEmitter&out){
        if(index>=4||!reference_units)return false;VantomTable table;
        if(!vantom_table(clip_id,milliseconds,loop,table))return false;
        const unsigned first=vantom_word(table.data,table.at+4);
        if(first>=table.words)return false;
        const unsigned count=vantom_word(table.data,first),base=first+1+count;
        if(count<2||count>513||base>table.words||count>(table.words-base)/(4*VantomUi15EmitterWords))return false;
        bone_scale=bone_scale<0?(table.row&128?512:1024):mx(512,mn(1024,bone_scale));
        unsigned low=0;while(low+2<count&&vantom_word(table.data,first+low+2)<=unsigned(bone_scale))++low;
        const unsigned lo=vantom_word(table.data,first+1+low),hi=vantom_word(table.data,first+2+low);
        if(lo>unsigned(bone_scale)||hi<unsigned(bone_scale)||hi<=lo)return false;
        const unsigned p=base+(low*4+index)*VantomUi15EmitterWords,q=p+4*VantomUi15EmitterWords;
        const auto read=[&](unsigned at){return int32_t(vantom_word(table.data,at)|(uint32_t(vantom_word(table.data,at+1))<<16));};
        int32_t values[6];for(unsigned i=0;i<6;++i){const int32_t left=read(p+i*2),right=read(q+i*2);
            values[i]=int32_t((int64_t(left)*(hi-bone_scale)+int64_t(right)*(bone_scale-lo))/(hi-lo));}
        const unsigned depth=vantom_word(table.data,p+12),flags=vantom_word(table.data,p+13);
        for(auto&value:values)value=int32_t(int64_t(value)*table.units/reference_units);
        out={values[0],values[1],values[2],values[3],values[4],values[5],uint16_t(depth),uint16_t(flags)};return true;
    }
    struct VantomDraw {int image,x,y,w,h;unsigned mode,depth;int mask=-1,geometry=-1;unsigned offset=0;int mesh_scale=0,mesh_bone=1024,continuation=-1;};
    bool vantom_collect(int clip_id,int x,int y,uint32_t milliseconds,int scale,bool loop,int bone_scale,
                        unsigned reference_units,VantomDraw*out,unsigned&visible,bool cull=true){
        if(scale<=0)return true;VantomTable table;if(!vantom_table(clip_id,milliseconds,loop,table))return false;
        const auto*data=table.data;const unsigned at=table.at,words=table.words;const int row=table.row;
        const unsigned start=vantom_word(data,at),count=vantom_word(data,at+1);
        if(count>VantomUi15MaxLayers||start>words||count>(words-start)/VantomUi15LayerWords)return false;
        const unsigned units=vantom_word(data,4)|(vantom_word(data,5)<<16);
        const int root_x=int16_t(vantom_word(data,6)),root_y=int16_t(vantom_word(data,7));
        const int tail_x=int16_t(vantom_word(data,at+2)),tail_y=int16_t(vantom_word(data,at+3));
        if(!units||!reference_units)return false;
        scale=int(int64_t(scale)*units/reference_units);
        bone_scale=bone_scale<0?(row&128?512:1024):mx(512,mn(1024,bone_scale));
        struct Layer {uint16_t image,mode,depth,mask,continuation;int16_t x,y;};Layer layers[VantomUi15MaxLayers];
        for(unsigned i=0;i<count;++i){const unsigned p=start+i*VantomUi15LayerWords;
            layers[i]={uint16_t(vantom_word(data,p)),uint16_t(vantom_word(data,p+3)),
                uint16_t(vantom_word(data,p+4)),uint16_t(vantom_word(data,p+5)),uint16_t(vantom_word(data,p+6)),
                int16_t(vantom_word(data,p+1)),int16_t(vantom_word(data,p+2))};
            if(layers[i].mode>7)return false;
        }
        // No parameter pointer survives the following Content calls.
        for(unsigned i=0;i<count;++i){const auto&layer=layers[i];
            const int image_id=layer.image;auto dimensions=a.content.image_dimensions(image_id);
            if(!dimensions.w||!dimensions.h)return false;
            const unsigned kind=layer.mode&3;
            if(kind==3){if(visible>=VantomUi15MaxLayers)return false;
                out[visible++]={image_id,x,y,0,0,unsigned((layer.mode>>2)&1),layer.depth,
                    layer.mask==65535?-1:int(layer.mask),int(uint16_t(layer.x)),unsigned(uint16_t(layer.y)),scale,bone_scale,
                    layer.continuation==65535?-1:int(layer.continuation)};continue;}
            if(layer.continuation!=65535)return false;
            int qx=layer.x*256,qy=layer.y*256,pixel_scale=scale;
            if(!kind){qx=root_x+(qx-root_x)*bone_scale/1024;qy=root_y+(qy-root_y)*bone_scale/1024;pixel_scale=scale*bone_scale/1024;}
            else if(kind==1){qx+=tail_x*(1024-bone_scale)/1024;qy+=tail_y*(1024-bone_scale)/1024;}
            const int left=x+vantom_round(int64_t(qx)*scale,262144),top=y+vantom_round(int64_t(qy)*scale,262144);
            const int width=mx(1,dimensions.w*pixel_scale/1024),height=mx(1,dimensions.h*pixel_scale/1024);
            if(cull&&(left>=clip_w||top>=clip_h||left+width<=0||top+height<=0))continue;
            if(visible>=VantomUi15MaxLayers)return false;
            out[visible++]={image_id,left,top,width,height,unsigned((layer.mode>>2)&1),layer.depth,layer.mask==65535?-1:int(layer.mask)};
        }
        return true;
    }
    // Expand one small source row, never a framebuffer. Mask alpha preserves
    // the original8-bit noise even when the pack chose sparse row encoding.
    static void vantom_pixels(Picture p,unsigned row,const uint8_t*&run,uint16_t*colors,uint8_t*alpha){
        if(p.format==4){
            for(unsigned x=0;x<p.w;++x)alpha[x]=0;
            const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
            for(unsigned i=0;i<spans;++i){const unsigned x=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8;
                const unsigned stride=run[4]?3u:2u;run+=5;
                for(unsigned j=0;j<n;++j){if(colors)colors[x+j]=run[0]|uint16_t(run[1])<<8;alpha[x+j]=stride==3?run[2]:255;run+=stride;}
            }
        }else for(unsigned x=0;x<p.w;++x){const unsigned at=row*p.w+x;
            alpha[x]=p.alpha?p.alpha[at]:255;if(colors){const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
                colors[x]=pixel[0]|uint16_t(pixel[1])<<8;}}
    }
    void vantom_masked(const VantomDraw&d,const uint8_t*threshold){
        const int ids[]={d.image,d.mask};a.content.prepare_images(ids,2);
        const auto p=a.content.image(d.image),mask=a.content.image(d.mask);
        if(!p.pixels||!mask.pixels||p.w>320||p.w!=mask.w||p.h!=mask.h)return;
        const int left=mx(0,-d.x),right=mn(d.w,clip_w-d.x),top=mx(0,-d.y),bottom=mn(d.h,clip_h-d.y);
        if(left>=right||top>=bottom)return;
        uint16_t colors[320],columns[320];uint8_t alpha[320],noise[320];
        const uint8_t*pixel_run=p.pixels,*mask_run=mask.pixels;unsigned decoded=0;
        for(int x=left;x<right;++x)columns[x-left]=uint16_t(unsigned(x)*p.w/unsigned(d.w));
        for(int y=top;y<bottom;++y){const unsigned row=unsigned(y)*p.h/unsigned(d.h);
            while(decoded<=row){vantom_pixels(p,decoded,pixel_run,colors,alpha);vantom_pixels(mask,decoded,mask_run,nullptr,noise);++decoded;}
            int at=(d.y+y)*pitch+d.x+left;
            for(int x=left;x<right;++x,++at){const unsigned source=columns[x-left],coverage=div255(alpha[source]*threshold[noise[source]]);
                if(coverage)paint(at,colors[source],coverage,d.mode);}
        }
    }
    // Only source meshes proved non-rigid use this bounded triangle path.
    // Geometry is copied before Content can recycle its word-image page.
    // The small texture is shared across phases; no intermediate bitmap lives
    // in App, State or a per-actor framebuffer.
    void vantom_mesh(const VantomDraw&d,const uint8_t*threshold){
        constexpr unsigned MaxVertices=VantomUi15MaxMeshVertices,MaxTriangles=VantomUi15MaxMeshTriangles,Header=16;
        struct Vertex {int32_t x,y,u,v;};Vertex vertices[MaxVertices];uint16_t triangles[MaxTriangles*3];
        static_assert(sizeof vertices+sizeof triangles<=2496,"Original Vantom topology must fit its 2.5 KiB mesh scratch");
        unsigned count=0,triangle_count=0,texture_w=0,texture_h=0;
        {
            const auto geometry=a.content.image(d.geometry);
            if(!geometry.pixels||geometry.format!=0||geometry.alpha||geometry.palette)return;
            const unsigned available=unsigned(geometry.w)*geometry.h;if(d.offset>available||Header>available-d.offset)return;
            const auto*data=geometry.pixels+d.offset*2;
            if(vantom_word(data,0)!=0x564d||vantom_word(data,1)!=1)return;
            count=vantom_word(data,2);triangle_count=vantom_word(data,3);
            const unsigned keys=vantom_word(data,4),keys_at=vantom_word(data,5),uv_at=vantom_word(data,6),
                triangles_at=vantom_word(data,7),xy_at=vantom_word(data,8),words=vantom_word(data,9);
            texture_w=vantom_word(data,10);texture_h=vantom_word(data,11);
            if(!count||count>MaxVertices||triangle_count>MaxTriangles||keys<2||keys>513||!texture_w||!texture_h||
               words>available-d.offset||keys_at>words||keys>words-keys_at||uv_at>words||count>(words-uv_at)/2||
               triangles_at>words||triangle_count>(words-triangles_at)/3||xy_at>words||keys>(words-xy_at)/(count*2))return;
            unsigned low=0;while(low+2<keys&&vantom_word(data,keys_at+low+1)<=unsigned(d.mesh_bone))++low;
            const unsigned lo=vantom_word(data,keys_at+low),hi=vantom_word(data,keys_at+low+1);
            if(hi<=lo||lo>unsigned(d.mesh_bone)||hi<unsigned(d.mesh_bone))return;
            for(unsigned i=0;i<count;++i){int coordinates[2];for(unsigned axis=0;axis<2;++axis){
                const int left=int16_t(vantom_word(data,xy_at+(low*count+i)*2+axis));
                const int right=int16_t(vantom_word(data,xy_at+((low+1)*count+i)*2+axis));
                coordinates[axis]=vantom_round(int64_t(left)*(hi-d.mesh_bone)+int64_t(right)*(d.mesh_bone-lo),int(hi-lo));}
                vertices[i]={d.x*64+vantom_round(int64_t(coordinates[0])*d.mesh_scale,1024),
                    d.y*64+vantom_round(int64_t(coordinates[1])*d.mesh_scale,1024),
                    int32_t(uint64_t(vantom_word(data,uv_at+i*2))*(texture_w-1)*4096/65535),
                    int32_t(uint64_t(vantom_word(data,uv_at+i*2+1))*(texture_h-1)*4096/65535)};
            }
            for(unsigned i=0;i<triangle_count*3;++i){triangles[i]=uint16_t(vantom_word(data,triangles_at+i));if(triangles[i]>=count)return;}
        }
        int ids[3]={d.image};unsigned image_count=1;if(d.mask>=0)ids[image_count++]=d.mask;
        if(d.continuation>=0)ids[image_count++]=d.continuation;a.content.prepare_images(ids,image_count);
        const auto texture=a.content.image(d.image),mask=d.mask>=0?a.content.image(d.mask):Picture{},
            continuation=d.continuation>=0?a.content.image(d.continuation):Picture{};
        const unsigned logical_pixels=texture_w*texture_h,first_pixels=d.continuation>=0?VantomUi15TexturePagePixels:logical_pixels;
        if(!texture.pixels||texture.format>3||unsigned(texture.w)*texture.h<first_pixels||
           (d.mask>=0&&(!mask.pixels||mask.format>3||unsigned(mask.w)*mask.h<logical_pixels))||
           (d.continuation>=0&&(!continuation.pixels||continuation.format>3||logical_pixels<=first_pixels||
             logical_pixels-first_pixels>unsigned(continuation.w)*continuation.h)))return;
        const auto texel=[&](unsigned at,unsigned&r,unsigned&g,unsigned&b,unsigned&alpha){
            const Picture&source=d.continuation>=0&&at>=first_pixels?continuation:texture;if(&source==&continuation)at-=first_pixels;
            const uint8_t*pixel=source.palette?source.palette+2*source.pixels[at]:source.pixels+2*at;
            const unsigned color=pixel[0]|unsigned(pixel[1])<<8;r=color>>11;g=(color>>5)&63;b=color&31;
            alpha=source.alpha?source.alpha[at]:255;
        };
        for(unsigned triangle=0;triangle<triangle_count;++triangle){
            Vertex v0=vertices[triangles[triangle*3]],v1=vertices[triangles[triangle*3+1]],v2=vertices[triangles[triangle*3+2]];
            int64_t area=int64_t(v1.x-v0.x)*(v2.y-v0.y)-int64_t(v1.y-v0.y)*(v2.x-v0.x);
            if(!area)continue;if(area<0){const auto temp=v1;v1=v2;v2=temp;area=-area;}
            const int left=mx(0,(mn(v0.x,mn(v1.x,v2.x))>>6)-1),right=mn(clip_w,((mx(v0.x,mx(v1.x,v2.x))+63)>>6)+1);
            const int top=mx(0,(mn(v0.y,mn(v1.y,v2.y))>>6)-1),bottom=mn(clip_h,((mx(v0.y,mx(v1.y,v2.y))+63)>>6)+1);
            if(left>=right||top>=bottom)continue;
            const int dx0=v2.x-v1.x,dy0=v2.y-v1.y,dx1=v0.x-v2.x,dy1=v0.y-v2.y,dx2=v1.x-v0.x,dy2=v1.y-v0.y;
            const int32_t ex0=-dy0*64,ey0=dx0*64,ex1=-dy1*64,ey1=dx1*64,ex2=-dy2*64,ey2=dx2*64;
            int64_t row0=int64_t(dx0)*(top*64+32-v1.y)-int64_t(dy0)*(left*64+32-v1.x);
            int64_t row1=int64_t(dx1)*(top*64+32-v2.y)-int64_t(dy1)*(left*64+32-v2.x);
            int64_t row2=int64_t(dx2)*(top*64+32-v0.y)-int64_t(dy2)*(left*64+32-v0.x);
            const bool inclusive0=dy0<0||(!dy0&&dx0>0),inclusive1=dy1<0||(!dy1&&dx1>0),inclusive2=dy2<0||(!dy2&&dx2>0);
            int64_t ru=(row0*v0.u+row1*v1.u+row2*v2.u)/area,rv=(row0*v0.v+row1*v1.v+row2*v2.v)/area;
            const int64_t ux=(int64_t(ex0)*v0.u+int64_t(ex1)*v1.u+int64_t(ex2)*v2.u)/area,
                vx=(int64_t(ex0)*v0.v+int64_t(ex1)*v1.v+int64_t(ex2)*v2.v)/area,
                uy=(int64_t(ey0)*v0.u+int64_t(ey1)*v1.u+int64_t(ey2)*v2.u)/area,
                vy=(int64_t(ey0)*v0.v+int64_t(ey1)*v1.v+int64_t(ey2)*v2.v)/area;
            for(int y=top;y<bottom;++y,row0+=ey0,row1+=ey1,row2+=ey2,ru+=uy,rv+=vy){
                int64_t e0=row0,e1=row1,e2=row2,u=ru,v=rv;int at=y*pitch+left;
                for(int x=left;x<right;++x,++at,e0+=ex0,e1+=ex1,e2+=ex2,u+=ux,v+=vx){
                    if(e0<0||(!e0&&!inclusive0)||e1<0||(!e1&&!inclusive1)||e2<0||(!e2&&!inclusive2))continue;
                    const int pu=mx(0,mn(int((texture_w-1)*4096),int(u))),pv=mx(0,mn(int((texture_h-1)*4096),int(v)));
                    const unsigned ix=unsigned(pu)>>12,iy=unsigned(pv)>>12,jx=mn(int(ix+1),int(texture_w)-1),jy=mn(int(iy+1),int(texture_h)-1);
                    const unsigned fx=(unsigned(pu)&4095)>>4,fy=(unsigned(pv)&4095)>>4;
                    const unsigned locations[]={iy*texture_w+ix,iy*texture_w+jx,jy*texture_w+ix,jy*texture_w+jx};
                    const unsigned weights[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
                    unsigned red=0,green=0,blue=0,alpha=0,noise=0;
                    for(unsigned k=0;k<4;++k){unsigned r,g,b,a;texel(locations[k],r,g,b,a);
                        red+=r*weights[k];green+=g*weights[k];blue+=b*weights[k];alpha+=a*weights[k];
                        if(mask.pixels)noise+=(mask.alpha?mask.alpha[locations[k]]:255)*weights[k];}
                    alpha=(alpha+32768)>>16;if(mask.pixels)alpha=div255(alpha*threshold[(noise+32768)>>16]);
                    if(alpha)paint(at,uint16_t(((red+32768)>>16)<<11|((green+32768)>>16)<<5|((blue+32768)>>16)),alpha,d.mode);
                }
            }
        }
    }
    bool vantom_frame(int clip_id,int x,int y,uint32_t milliseconds,int scale,bool loop,int bone_scale=-1,unsigned actor=0){
        if(vantom_ui15_row(clip_id)<0)return false;if(scale<=0)return true;
        const unsigned units=a.content.clip(clip_id).sprite_units;
        VantomDraw draw[VantomUi15MaxLayers];unsigned visible=0;
        if(!vantom_collect(clip_id,x,y,milliseconds,scale,loop,bone_scale,units,draw,visible))return false;
        if(actor) vantom_vfx_sync(actor,clip_id,milliseconds,loop,bone_scale);
        const unsigned dissolve=actor?vantom_dissolve(actor):0;
        uint8_t threshold[256];for(unsigned i=0;i<256;++i){const int t=mx(0,mn(4096,(int(i)*4096-int(dissolve)*255)*10/255));
            threshold[i]=uint8_t((int64_t(t)*t*(12288-2*t)*255+34359738368ll)/68719476736ll);}
        int images[4*VantomUi15MaxLayers];unsigned images_count=0;
        for(unsigned i=0;i<visible;++i){images[images_count++]=draw[i].image;if(draw[i].mask>=0)images[images_count++]=draw[i].mask;
            if(draw[i].geometry>=0)images[images_count++]=draw[i].geometry;
            if(draw[i].continuation>=0)images[images_count++]=draw[i].continuation;}
        a.content.prepare_images(images,images_count);
        VantomEmitter emitters[4];unsigned pending=0;
        if(actor)for(unsigned i=0;i<4;++i)if(vantom_emitter(clip_id,milliseconds,loop,i,bone_scale,units,emitters[i]))pending|=1u<<i;
        for(unsigned i=0;i<visible;++i){const auto&d=draw[i];
            for(unsigned e=0;e<4;++e)if((pending&(1u<<e))&&emitters[e].depth<=d.depth){
                vantom_draw_particles(actor,e,x,y,scale,units);pending&=~(1u<<e);}
            if(d.geometry>=0){if(d.mask<0||dissolve<4096)vantom_mesh(d,threshold);}
            else if(d.mask>=0){if(dissolve<4096) vantom_masked(d,threshold);}else picture(d.image,d.x,d.y,d.w,d.h,255,d.mode);
        }
        for(unsigned e=0;e<4;++e)if(pending&(1u<<e))vantom_draw_particles(actor,e,x,y,scale,units);
        return true;
    }
