"""Pinned component metadata, not original image or localized text payloads.

Sizes adapt the authored scenes to the 320x240 display. Health caps retain
their native pixels; the tooltip's original nine-patch margins are scaled
along with its image. HSV triples use the reference YIQ shader, not colorsys.
"""
BASE_AUXILIARY_COUNT = 1175
REFERENCE = 0x41cef1ea
SKINS = (
    ('HEALTH_BAR_BG', 'images/ui/combat/health_bar_bg.png', (20, 16), None),
    ('HEALTH_BAR_STROKE', 'images/ui/combat/health_bar_stroke.png', (22, 16), None),
    ('HEALTH_BAR_MASK', 'images/ui/combat/health_bar.png', (12, 10), None),
    ('HEALTH_BAR_FILL', 'images/ui/combat/health_bar_fill.png', (15, 16), None),
    ('HEALTH_BAR_INFINITY', 'images/ui/combat/combat_infinity_hp.png', (50, 28), None),
    ('HEALTH_BAR_BLOCK', 'images/ui/combat/block.png', (32, 32), None),
    ('HOVER_TIP', 'images/ui/hover_tip.png', (170, 54), None),
    ('HOVER_TIP_DEBUFF', 'images/ui/hover_tip.png', (170, 54), (.47, 2., .9)),
    ('PAUSE_BUTTON', 'images/ui/reward_screen/reward_item_button.png', (186, 40), (1., .8, .9)),
    ('PAUSE_BUTTON_HOVER', 'images/ui/reward_screen/reward_item_button.png', (186, 40), (1., 1.1, 1.1)),
    ('END_TURN', 'images/packed/combat_ui/end_turn_button.png', (128, 64), None),
    ('END_TURN_HOVER', 'images/packed/combat_ui/end_turn_button.png', (128, 64), (1., 1., 1.5)),
    ('END_TURN_GLOW', 'images/packed/combat_ui/end_turn_button_glow.png', (128, 64), None),
)
AUXILIARY_COUNT = BASE_AUXILIARY_COUNT + len(SKINS)
