static unsigned card_enchant_smooth(unsigned value,unsigned low,unsigned high){
 if(value<=low)return 0;if(value>=high)return 4096;
 const unsigned t=(value-low)*4096/(high-low);return t*t/4096*(12288-2*t)/4096;
}
static unsigned card_enchant_progress(uint32_t age){
 if(age>=1000)return 4096;
 const unsigned t=age*4096/1000;return t<2048?2*t*t/4096:4096-2*(4096-t)*(4096-t)/4096;
}
struct CardEnchantViewport {
 const Content*owner=nullptr;uint32_t generation=0,start=0;unsigned kind=0;int token=-1,amount=0,width=0,height=0;
 uint16_t pixels[44*34]{};uint8_t alpha[44*34]{};char description[1024]{};
};
static CardEnchantViewport enchantment_viewports[RewardPresentation::Enchants::Columns];
static CardEnchantViewport& card_enchant_viewport(Canvas&c,const RewardPresentation::Enchants::CardView&model,unsigned slot,int card_w){
 auto&t=enchantment_viewports[slot%RewardPresentation::Enchants::Columns];const auto&facts=model.model.facts;
 const int card_h=card_w*HandFaceH/HandFaceW,w=card_face::scaled(144,card_w,300),h=card_face::scaled(108,card_h,422);
 if(t.owner==&c.a.content&&t.generation==c.a.content.generation()&&t.start==model.start&&t.token==card_token(model.model.card)&&t.kind==unsigned(facts.enchantment)&&t.amount==facts.enchantment_amount&&t.width==w&&t.height==h)return t;
 t.owner=&c.a.content;t.generation=c.a.content.generation();t.start=model.start;t.token=card_token(model.model.card);t.kind=unsigned(facts.enchantment);t.amount=facts.enchantment_amount;t.width=w;t.height=h;
 for(unsigned i=0;i<44*34;++i){t.pixels[i]=0;t.alpha[i]=0;}
 Canvas layer{c.a,t.pixels,44,w,h,t.alpha};
 // NCardEnchantVfx renders a144x108 transparent viewport containing the
 // 72x54 tab at(36,27). Its material uses v=1 rather than the resting.6.
 static constexpr int16_t enchant_matrix[]={955,1875,1282,1760,2342,-13,-827,4118,800};
 const auto tab=card_face::region(0,0,card_w,card_h,36,27,72,54);
 Canvas background=layer;background.color_matrix=enchant_matrix;background.ui(I13_CardEnchantment,tab.x,tab.y,tab.w,tab.h);
 const auto icon=card_face::region(0,0,card_w,card_h,50,36,35,35);
 layer.picture(c.a.content.world_image("enchantments",event_enchantment_key(facts.enchantment)),icon.x,icon.y,icon.w,icon.h);
 bool label=false;switch(facts.enchantment){case EventEnchantment::Sharp:case EventEnchantment::Nimble:case EventEnchantment::Swift:
  case EventEnchantment::Momentum:case EventEnchantment::Adroit:case EventEnchantment::Vigorous:label=true;break;default:break;}
 if(label){const auto r=card_face::region(0,0,card_w,card_h,62,54,40,26);char number[16]{};TextBuffer text{number,sizeof number,0};text.number(facts.enchantment_amount);
  layer.font_face=uint8_t(source_label_face(c.a.content.original_label_fonts(),5));
  layer.text_fitted(number,r.x,r.y,mx(8,r.w),8,rgb(255,246,226),8,true,true,rgb(9,15,28));}
 c.a.world_card_description(model.model.card,-1,t.description,sizeof t.description,-1,-1,&facts,&model.model);return t;
}
static void card_enchant_tab_appear(Canvas&c,const CardEnchantViewport&t,int x,int y,uint32_t age){
 const unsigned progress=card_enchant_progress(age);
 for(int yy=0;yy<t.height;++yy)for(int xx=0;xx<t.width;++xx){
  const unsigned u=unsigned(2*xx+1)*2048/unsigned(t.width),v=unsigned(2*yy+1)*2048/unsigned(t.height);
  const unsigned white=card_enchant_smooth(u,progress,progress+819),fade=card_enchant_smooth(u,progress+819,progress+1024);
  const int vv=2048+(int(v)-2048)*(4096-int(white)/4)/4096;
  const int sy=mn(t.height-1,mx(0,vv*t.height/4096)),at=sy*44+xx;const unsigned tex_alpha=t.alpha[at];if(!tex_alpha)continue;
  const unsigned mix_alpha=tex_alpha+(255-tex_alpha)*white/4096;
  const unsigned alpha=div255((mix_alpha*(4096-fade)/4096)*tex_alpha);if(!alpha)continue;
  const unsigned base=t.pixels[at],r=(base>>11)*255/31,g=(base>>5&63)*255/63,b=(base&31)*255/31;
  const uint16_t color=rgb(mn(255,int(r+white*255/4096)),mn(255,int(g+white*191/4096)),mn(255,int(b+white*230/4096)));
  if(x+xx>=0&&x+xx<c.clip_w&&y+yy>=0&&y+yy<c.clip_h)c.paint((y+yy)*c.pitch+x+xx,color,alpha);
 }
}
static void card_enchant_sparkles(Canvas&c,int x,int y,int card_w,uint32_t age,uint32_t seed){
 if(age<200)return;
 for(unsigned i=0;i<20;++i){const unsigned start=200+i*20;if(age<start)continue;const unsigned t=age-start;if(t>=2000)continue;
  const uint32_t hash=room_vfx_hash(seed+i*0x9e3779b9u);const int velocity=40+int(hash%21);
  const int angle=int((hash>>8)%1025)-512,travel=int(t)*velocity/1000;
  const int source_x=-158+int(i*72/20)+CombatCardMotion::sine(angle+1024)*travel/1024;
  const int source_y=-93+int((hash>>19)%21)-10+CombatCardMotion::sine(angle)*travel/1024-int(t*t/20000);
  const int px=x+source_x*card_w/300,py=y+source_y*card_w/300;
  const unsigned q=t*4096/2000,fade=4096-q*q/4096;
  // The authored scale rises quickly, then holds near half size.
  const unsigned size_q=t<172?t*4096/172:t<392?4096-(t-172)*1966/220:2130;
  const int size=mx(1,13*card_w*int(size_q)/(300*4096));
  Canvas spark=c;spark.tint_green=253;spark.tint_blue=204;
  card_effect_texture(spark,c.a.content.ui_image(I14_EnchantSpark),px,py,size,size,0,1024,fade*255/4096,presentation::Add);
 }
}
static void enchanted_card_animation(Canvas&c){
 auto&a=c.a;auto&m=a.reward_presentation.enchants;if(!m.alive(a.map_clock_ms))return;
 for(unsigned i=0;i<m.count;++i){auto&card=m.cards[i];if(int32_t(a.map_clock_ms-card.start)<0)continue;
  const uint32_t age=a.map_clock_ms-card.start;if(age>=2600)continue;
  const unsigned first=i/RewardPresentation::Enchants::Columns*RewardPresentation::Enchants::Columns;
  const unsigned columns=mn(RewardPresentation::Enchants::Columns,unsigned(m.count)-first);
  const int width=columns>3?63:columns>1?76:HandFaceW;
  const int x=160+(2*int(i-first)-int(columns)+1)*(width+8)/2,y=117;
  if(age>=2000){if(!card.launched){
    a.reward_presentation.append(RewardPickupKind::Card,card.model.card.kind,unsigned(card.deck),x,y,card.start+2000,card.model.card,&card.model);card.launched=true;
   }continue;}
  auto&viewport=card_enchant_viewport(c,card,i-first,width);
  EventCardFacts hidden_tab=card.model.facts;hidden_tab.enchantment=EventEnchantment::None;hidden_tab.enchantment_amount=0;
  CombatCardMotion::Sprite sprite;sprite.card=card.model.card;sprite.energy=card.energy;sprite.stars=card.stars;
  const auto&tile=hand_tile(c,-1,&sprite,-1,&hidden_tab,false,viewport.description);
  draw_hand_tile(c,tile,{x,y,width*1024/HandFaceW,0,1024},false);
  const int height=width*HandFaceH/HandFaceW;
  const auto r=card_face::region(x-width/2,y-height/2,width,height,-52,69,144,108);
  card_enchant_tab_appear(c,viewport,r.x,r.y,age);
  card_enchant_sparkles(c,x,y,width,age,card.start^unsigned(card_token(card.model.card)));
 }
}
