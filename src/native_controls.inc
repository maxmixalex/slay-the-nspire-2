// Original component artwork preserves its aspect ratio, including the
// authored transparent margins. Callers own the larger readable hit target.
static void native_control(Canvas&c,const char*kind,int x,int y,int w,int h,const char*label,bool enabled,bool selected){
 const int image=c.a.content.world_image("common",kind);const unsigned opacity=enabled?255:100;
 if(image>=0){
  const auto info=c.a.content.image(image);const int height=info.w?int(w*info.h/info.w):h,top=y+(h-height)/2;
  c.picture(image,x,top,w,height,opacity);
  char key[48]{};TextBuffer b{key,sizeof key,0};b.text(kind);b.text("_OUTLINE");
  if(selected&&enabled){int outline=c.a.content.world_image("common",key);if(outline>=0)c.picture(outline,x,top,w,height);}
 }else c.ui(U_RewardSkip,x,y,w,h,opacity);
 int label_x=x+3,label_w=w-6;
 if(image>=0&&(kind[0]=='B'||kind[0]=='C')){int icon=c.a.content.world_image("common",kind[0]=='B'?"BACK_ARROW":"CONFIRM_TICK");
  if(icon>=0){c.picture(icon,x+10,y+(h-15)/2,15,15,opacity);label_x=x+28;label_w=w-36;}}
 c.centered(label,label_x,y+(h-12)/2,label_w,12,enabled?Gold:Muted);
}
static void native_proceed(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"PROCEED",x,y,w,h,label,enabled,selected);}
static void native_back(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"BACK",x,y,w,h,label,enabled,selected);}
static void native_confirm(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"CONFIRM",x,y,w,h,label,enabled,selected);}
