template<class Visit>static int auxiliary_draft_layout(const App&a,Visit visit){
 const char*start=a.auxiliary.draft;int x=0,row=0;
 for(const char*p=start;*p;){const unsigned at=unsigned(p-start);const unsigned cp=utf8(p);
  if(cp=='\n'){visit(at,cp,x,row);x=0;++row;continue;}
  const int advance=a.content.glyph_metrics(cp,8).advance;
  if(x&&x+advance>126){x=0;++row;}visit(at,cp,x,row);x+=advance;
 }
 visit(a.auxiliary.draft_size,0,x,row);return row+1;
}
static void auxiliary_reveal_caret(App&a){
 auto&v=a.auxiliary;int row=0;
 auxiliary_draft_layout(a,[&](unsigned at,unsigned,int,int line){if(at==v.caret)row=line;});
 if(row<int(v.draft_scroll))v.draft_scroll=uint16_t(row);
 else if(row>=int(v.draft_scroll)+4)v.draft_scroll=uint16_t(row-3);
}
static void auxiliary_measure_note(App&a){
 auto&v=a.auxiliary;if(v.screen!=AuxiliaryScreen::PatchNotes||v.measured_note==int(v.note))return;
 Canvas measure{a};measure.bold_face=5;measure.wrap_latin_words=true;const auto&note=AuxiliaryNotes[v.note];int total=0;
 for(unsigned i=0;i<note.chunks;++i){const char*body=a.content.string(note.text+i);
  int lines=*body?measure.paragraph(body,0,0,184,0,0,8):0;unsigned length=0;while(body[length])++length;
  if(i+1<note.chunks&&length&&body[length-1]=='\n'&&lines)--lines;
  v.note_lines[i]=uint16_t(lines);total+=lines;
 }
 v.scroll.configure(28+total*11+23,240);v.measured_note=int16_t(v.note);
}
void App::open_auxiliary(AuxiliaryScreen screen){
 if(screen==AuxiliaryScreen::None){close_auxiliary();return;}
 if(frontend_confirm||binding_listening>=0)return;
 if(screen==AuxiliaryScreen::PatchNotes&&(!title||settings_open||profiles_open||compendium_open||library||character_select||run_mode.screen!=RunMenu::Closed))return;
 auto&v=auxiliary;if(v.screen==screen){close_auxiliary();return;}
 v.screen=screen;v.opened=map_clock_ms;v.closing=false;v.scroll.reset();v.measured_note=-1;
 v.credits_target=v.credits_position=0;v.credits_remainder=0;
 v.editing=v.category_open=false;v.focus=0;v.export_result=0;
 if(screen==AuxiliaryScreen::Feedback){
  if(map_clock_ms-v.closed>60000u){v.draft[0]=0;v.draft_size=v.draft_units=v.caret=v.draft_scroll=0;v.emoji=-1;}
  if(content.english()&&v.category>2)v.category=2;
 }
 auxiliary_measure_note(*this);pointer_cancel();physical_keys_cancel();keyword_hover={};
}
void App::close_auxiliary(){
 auto&v=auxiliary;if(v.screen==AuxiliaryScreen::None||v.closing)return;
 if(v.screen==AuxiliaryScreen::Credits&&map_clock_ms-v.opened<2000u)return;
 pointer_cancel();v.editing=v.category_open=false;v.closed=map_clock_ms;
 if(v.screen==AuxiliaryScreen::Feedback){v.screen=AuxiliaryScreen::None;return;}
 v.closing=true;v.closing_at=map_clock_ms;
}
void App::advance_auxiliary(unsigned ms){
 auto&v=auxiliary;if(v.screen==AuxiliaryScreen::None)return;
 if(v.closing&&map_clock_ms-v.closing_at>=(v.screen==AuxiliaryScreen::Credits?1000u:250u)){v.screen=AuxiliaryScreen::None;v.closing=false;return;}
 if(v.screen==AuxiliaryScreen::Credits){
  //80 original source pixels per second, projected by4/21. Fixed-point
  // residue retains subpixel motion without a platform-dependent float.
  v.credits_remainder+=ms*80u*4u*256u;v.credits_target+=int(v.credits_remainder/21000u);v.credits_remainder%=21000u;
  const int delta=v.credits_target-v.credits_position,weight=mn(1000,int(ms)*20);
  v.credits_position+=int(int64_t(delta)*weight/1000);
 }else if(v.screen==AuxiliaryScreen::PatchNotes)v.scroll.step(ms);
}
void App::auxiliary_scroll(int rows){
 auto&v=auxiliary;if(v.closing||!rows)return;
 if(v.screen==AuxiliaryScreen::Credits)v.credits_target+=rows*50*4*256/21;
 else if(v.screen==AuxiliaryScreen::PatchNotes){auxiliary_measure_note(*this);v.scroll.wheel(rows);}
 else if(v.screen==AuxiliaryScreen::Feedback){const int count=auxiliary_draft_layout(*this,[](unsigned,unsigned,int,int){});
  v.draft_scroll=uint16_t(mx(0,mn(mx(0,count-4),int(v.draft_scroll)+rows)));}
}
unsigned App::feedback_report(char*out,unsigned capacity)const{
 if(!out||capacity<auxiliary.draft_size+512u)return 0;
 static const char*const categories[]={"bug","balance","feedback","translation"};
 static const char*const emojis[]={"happy_cultist","heart","slime_sad","exclaim","question"};
 TextBuffer text{out,capacity,0};text.text("Slay the nSpire2 local feedback report\nPort: 0.17\nReference: v0.111.0 / 41cef1ea\nLanguage: ");
 text.text(content.english()?"eng":"zhs");text.text("\nCategory: ");text.text(categories[auxiliary.category<4?auxiliary.category:2]);
 text.text("\nEmoji: ");text.text(auxiliary.emoji>=0&&auxiliary.emoji<5?emojis[auxiliary.emoji]:"none");
 text.text("\nProfile: ");text.number(callbacks.current_profile?int(callbacks.current_profile()):1);
 text.text("\nRun active: ");text.text(title?"no":"yes");
 if(!title){text.text("\nCharacter: ");text.text(character_definitions[unsigned(game.character)].key);
  text.text("\nFloor: ");text.number(game.world.active?game.world.total_floor:game.floor);text.text("\nPhase: ");text.number(int(game.phase));}
 text.text("\n\n");text.text(auxiliary.draft);text.ch('\n');return unsigned(text.n);
}
void App::auxiliary_control(int control){
 if(control==AuxiliaryOpenPatchNotes){open_auxiliary(AuxiliaryScreen::PatchNotes);return;}
 if(control==AuxiliaryOpenCredits){open_auxiliary(AuxiliaryScreen::Credits);return;}
 if(control==AuxiliaryOpenFeedback){open_auxiliary(AuxiliaryScreen::Feedback);return;}
 auto&v=auxiliary;if(v.screen==AuxiliaryScreen::None||v.closing)return;
 if(control==AuxiliaryBack){close_auxiliary();return;}
 if(control==AuxiliaryScrollUp||control==AuxiliaryScrollDown){auxiliary_scroll(control==AuxiliaryScrollUp?-3:3);return;}
 if(v.screen==AuxiliaryScreen::PatchNotes&&(control==AuxiliaryPrevious||control==AuxiliaryNext)){
  const int next=int(v.note)+(control==AuxiliaryPrevious?1:-1);if(next<0||next>=int(AuxiliaryNoteCount))return;
  v.note=uint16_t(next);v.scroll.reset();v.measured_note=-1;auxiliary_measure_note(*this);pointer_cancel();return;
 }
 if(v.screen!=AuxiliaryScreen::Feedback)return;
 if(control!=AuxiliaryFeedbackText)v.editing=false;
 if(control>=AuxiliaryFeedbackEmojiFirst&&control<AuxiliaryFeedbackEmojiFirst+5){
  const int selected=control-AuxiliaryFeedbackEmojiFirst;v.emoji=int8_t(v.emoji==selected?-1:selected);v.focus=uint8_t(selected+2);v.export_result=0;return;}
 if(control>=AuxiliaryFeedbackCategoryFirst&&control<AuxiliaryFeedbackCategoryFirst+(content.english()?3:4)){
  v.category=uint8_t(control-AuxiliaryFeedbackCategoryFirst);v.category_open=false;v.focus=1;v.export_result=0;return;}
 if(control==AuxiliaryFeedbackCategory){v.category_open=!v.category_open;v.focus=1;return;}
 v.category_open=false;
 if(control==AuxiliaryFeedbackText){v.editing=true;v.focus=0;
  if(pointer.visible){const int row=mx(0,(pointer.y-70)/11)+v.draft_scroll,x=mx(0,pointer.x-97);unsigned best=v.draft_size;int distance=0x7fffffff;
   auxiliary_draft_layout(*this,[&](unsigned at,unsigned,int xx,int line){int dy=line-row,dx=xx-x;if(dy<0)dy=-dy;if(dx<0)dx=-dx;const int score=dy*1000+dx;if(score<distance){distance=score;best=at;}});v.caret=uint16_t(best);}
  auxiliary_reveal_caret(*this);return;
 }
 if(control==AuxiliaryFeedbackExport){
  // This port exports a local draft. It never calls the original feedback
  // service and never reports an external submission as successful.
  char report[24576]{};const unsigned length=feedback_report(report,sizeof report);
  v.export_result=length&&callbacks.write_feedback&&callbacks.write_feedback(report,length)?1:2;v.focus=7;
 }
}
bool App::auxiliary_text(int code){
 auto&v=auxiliary;if(v.screen!=AuxiliaryScreen::Feedback||!v.editing)return false;
 if(code==27||code==9){v.editing=false;v.focus=code==9?1:0;return true;}
 const auto previous=[&](unsigned at){if(at){do{--at;}while(at&&(uint8_t(v.draft[at])&0xc0)==0x80);}return at;};
 const auto next=[&](unsigned at){if(at<v.draft_size){++at;while(at<v.draft_size&&(uint8_t(v.draft[at])&0xc0)==0x80)++at;}return at;};
 if(code==-1)v.caret=uint16_t(previous(v.caret));
 else if(code==-2)v.caret=uint16_t(next(v.caret));
 else if(code==-3||code==-4||code==-5||code==-6){
  int row=0,x=0;auxiliary_draft_layout(*this,[&](unsigned at,unsigned,int xx,int yy){if(at==v.caret){row=yy;x=xx;}});
  const int wanted=mx(0,row+(code==-3?-1:code==-4?1:0)),target=code==-5?0:code==-6?10000:x;unsigned best=v.caret;int distance=0x7fffffff;
  auxiliary_draft_layout(*this,[&](unsigned at,unsigned,int xx,int yy){int dy=yy-wanted,dx=xx-target;if(dy<0)dy=-dy;if(dx<0)dx=-dx;const int score=dy*10000+dx;if(score<distance){distance=score;best=at;}});v.caret=uint16_t(best);
 }else if(code==8||code==127){
  const unsigned start=code==8?previous(v.caret):v.caret,end=code==8?v.caret:next(v.caret);
  if(end>start){v.draft_units-=uint8_t(v.draft[start])>=0xf0?2:1;for(unsigned i=end;i<=v.draft_size;++i)v.draft[i-(end-start)]=v.draft[i];v.draft_size-=uint16_t(end-start);v.caret=uint16_t(start);}
 }else if(code==10||code==13||(code>=32&&code<=0x10ffff&&!(code>=0xd800&&code<=0xdfff))){
  if(code==13)code=10;const unsigned units=code>=0x10000?2:1;if(v.draft_units+units>8000)return true;
  char bytes[4];unsigned n=0;
  if(code<0x80)bytes[n++]=char(code);else if(code<0x800){bytes[n++]=char(0xc0|(code>>6));bytes[n++]=char(0x80|(code&63));}
  else if(code<0x10000){bytes[n++]=char(0xe0|(code>>12));bytes[n++]=char(0x80|((code>>6)&63));bytes[n++]=char(0x80|(code&63));}
  else{bytes[n++]=char(0xf0|(code>>18));bytes[n++]=char(0x80|((code>>12)&63));bytes[n++]=char(0x80|((code>>6)&63));bytes[n++]=char(0x80|(code&63));}
  if(v.draft_size+n>=sizeof v.draft)return true;
  for(unsigned i=v.draft_size+1;i>v.caret;--i)v.draft[i+n-1]=v.draft[i-1];
  for(unsigned i=0;i<n;++i)v.draft[v.caret+i]=bytes[i];v.draft_size+=uint16_t(n);v.caret+=uint16_t(n);v.draft_units+=uint16_t(units);
 }else return true;
 v.export_result=0;auxiliary_reveal_caret(*this);return true;
}
bool App::auxiliary_input(Key key){
 auto&v=auxiliary;if(v.screen==AuxiliaryScreen::None)return false;if(v.closing)return true;
 if(v.screen==AuxiliaryScreen::Feedback&&v.editing){
  if(key==Key::Back)auxiliary_text(27);else if(key==Key::Confirm)auxiliary_text(13);
  else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)auxiliary_text(key==Key::Left?-1:key==Key::Right?-2:key==Key::Up?-3:-4);
  return true;
 }
 if(key==Key::Back){if(v.category_open)v.category_open=false;else close_auxiliary();return true;}
 if(v.screen==AuxiliaryScreen::Credits){if(key==Key::Confirm)close_auxiliary();else if(key==Key::Up||key==Key::Down)auxiliary_scroll(key==Key::Up?-3:3);return true;}
 if(v.screen==AuxiliaryScreen::PatchNotes){
  if(key==Key::Left||key==Key::Right)auxiliary_control(key==Key::Left?AuxiliaryPrevious:AuxiliaryNext);
  else if(key==Key::Up||key==Key::Down)auxiliary_scroll(key==Key::Up?-3:3);return true;
 }
 if(v.category_open){
  if(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right){const int count=content.english()?3:4;v.category=uint8_t((v.category+count+(key==Key::Up||key==Key::Left?-1:1))%count);}
  else if(key==Key::Confirm)v.category_open=false;return true;
 }
 if(key==Key::Up||key==Key::Left)v.focus=uint8_t((v.focus+7)%8);
 else if(key==Key::Down||key==Key::Right)v.focus=uint8_t((v.focus+1)%8);
 else if(key==Key::Confirm)auxiliary_control(v.focus==0?AuxiliaryFeedbackText:v.focus==1?AuxiliaryFeedbackCategory:v.focus==7?AuxiliaryFeedbackExport:AuxiliaryFeedbackEmojiFirst+v.focus-2);
 return true;
}
