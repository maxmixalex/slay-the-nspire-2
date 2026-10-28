"""0.8 small-screen assets, sourced from the pinned original scenes.

Order is part of NSP2/5. Keep include/ui_assets.hpp in sync. No source art is
stored in the repository; only the local converter reads the user's PCK.
"""
UI_ASSETS = [
 ('TitleBackground', '#mainmenu', (320,240), 'cover'),
 ('CharacterBackground', '#ironclad', (320,240), 'cover'),
 ('BattleBackground', '#battle', (320,218), 'cover'),
 ('RewardPanel', 'reward_panel.png', (220,192), 'stretch'),
 ('RewardBanner', 'reward_banner.png', (244,61), 'contain'),
 ('RewardButton', 'reward_item_button.png', (180,29), 'stretch'),
 ('RewardCard', 'reward_icon_card.png', (23,23), 'contain'),
 ('RewardGold', 'reward_icon_money.png', (23,23), 'contain'),
 ('RewardSkip', 'reward_skip_button.png', (91,25), 'stretch'),
 ('RestHeal', 'option_heal.png', (80,57), 'contain'),
 ('RestSmith', 'option_smith.png', (80,57), 'contain'),
 ('RestOutline', 'option_outline.png', (86,61), 'contain'),
 ('ShopRug', 'shop_rug.png', (312,196), 'stretch'),
 ('TopBar', '@top_bar/top_bar', (320,22), 'stretch'),
 ('TopDeck', '@top_bar/top_bar_deck', (18,18), 'contain'),
 ('TopMap', '@top_bar/top_bar_map', (18,18), 'contain'),
 ('TopSettings', '@top_bar/top_bar_settings', (17,17), 'contain'),
 ('TopFloor', '@top_bar/top_bar_floor', (12,14), 'contain'),
 ('CharPortrait', 'char_select_ironclad.png', (36,40), 'contain'),
 ('LibraryCards', 'submenu_card_library.png', (26,26), 'contain'),
 ('LibraryRelics', 'submenu_relic_collection.png', (26,26), 'contain'),
 ('LibraryPotions', 'submenu_potion_lab.png', (26,26), 'contain'),
 ('ColorlessEnergy', '@card/energy_colorless', (26,26), 'contain'),
 *[(name,'images/relics/'+key+'.png',(25,25),'contain') for name,key in [
  ('Strawberry','strawberry'),('Pear','pear'),('Mango','mango'),
  ('Marbles','bag_of_marbles'),('Akabeko','akabeko'),('BronzeScales','bronze_scales')]],
 *[(name,key+'.png',(23,28),'contain') for name,key in [
  ('EnergyPotion','energy_potion'),('StrengthPotion','strength_potion'),('DexterityPotion','dexterity_potion'),
  ('SwiftPotion','swift_potion'),('ExplosivePotion','explosive_ampoule'),('VulnerablePotion','vulnerable_potion'),
  ('WeakPotion','weak_potion'),('FruitPotion','fruit_juice'),('ForgePotion','blessing_of_the_forge'),
  ('FlexPotion','flex_potion'),('SpeedPotion','speed_potion'),('BronzePotion','liquid_bronze'),('Fortifier','fortifier')]],
 ('BigAttack','@card/card_frame_attack_s',(80,112),'stretch'),
 ('BigSkill','@card/card_frame_skill_s',(80,112),'stretch'),
 ('BigPower','@card/card_frame_power_s',(80,112),'stretch'),
 ('BigBanner','@card/card_banner',(80,17),'stretch'),
 *[(name,key+'_power.png',(13,13),'contain') for name,key in [
  ('Strength','strength'),('Dexterity','dexterity'),('Vigor','vigor'),('Thorns','thorns'),
  ('Weak','weak'),('Vulnerable','vulnerable'),('NoDraw','no_draw')]],
]
UI_ASSETS.extend([('CommonBanner','!banner:common',(80,17),'stretch'),
                  ('RareBanner','!banner:rare',(80,17),'stretch')])
UI_ASSETS.append(('CampFlames','#campflames',(46,58),'contain'))
COLORLESS_CARDS=['FINESSE','FLASH_OF_STEEL']
