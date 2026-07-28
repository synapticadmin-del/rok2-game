# رسم تخطيطي للماب (ASCII Layout)

## الصورة الكاملة

```
#####################################################################
#                         WORLD BORDER / OCEAN                        #
#                                                                     #
#   Z1-R1 ##### P ##### Z1-R2 ##### P ##### Z1-R3                     #
#     #  mountains  #         mountains  #                            #
#     P             P                     P                           #
#     #             #                     #                           #
#   Z1-R8          ... inner ring ...   Z1-R4                         #
#     #             #                     #                           #
#     P             P                     P                           #
#     #  mountains  #         mountains  #                            #
#   Z1-R7 ##### P ##### Z1-R6 ##### P ##### Z1-R5                     #
#                                                                     #
#          \\\ Inner Passes (Lv3-4) to Zone2 ///                      #
#                                                                     #
#              Z2-W #### P #### Z2-N #### P #### Z2-E                 #
#                #   mountains/ring walls   #                         #
#                P                           P                        #
#                #         Z2-S              #                         #
#                                                                     #
#                    \\\ Final Gates ///                              #
#                                                                     #
#                    +-------------------+                            #
#                    |     ZONE 3        |                            #
#                    |  outer forts      |                            #
#                    |   [CORE/THRONE]   |                            #
#                    +-------------------+                            #
#                                                                     #
#####################################################################
```

## تدفق التوسع للاعب/التحالف

```
Spawn in one of 8 Z1 regions
        ↓
Secure local resources + altar
        ↓
Take border passes → neighbor regions
        ↓
Contest Inner Pass → enter Zone2
        ↓
Hold shrines + staging territory
        ↓
Break Final Gate → Zone3
        ↓
Contest Core for finals ranking
```

## عدّاد الكيانات (Target Design)

| الطبقة | Regions | Passes (تقريبي) | Core objectives |
|--------|--------:|----------------:|----------------:|
| Zone1 | 8 | 12–24 | 8 altars |
| Zone2 | 4 | 8–12 | 8–12 shrines/forts |
| Zone3 | 1 | 2–4 gates | 1 core + 4–8 side |
| **Total** | **13** | **~25–40** | **~25–30** |

هذا الحجم ممتاز لـ MVP موسمي بدون ما تتعقد كـ RoK بكل سيناريوهات LostLand_Map_20.
