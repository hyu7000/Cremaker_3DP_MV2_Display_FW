#include "AddZOffsetCal.h"
#include "includes.h"

ADDZOFFSETCAL addZOffsetCal;

// 1 title, ITEM_PER_PAGE items (icon + label)
MENUITEMS addZOffsetCalItems = {
    // title
    LABEL_LEVELING_ADD_Z,
    // icon                         label
    {
      {ICON_BACKGROUND,         LABEL_BACKGROUND},
      {ICON_BACKGROUND,         LABEL_BACKGROUND},
      {ICON_RESUME,             LABEL_TEST},
      {ICON_PAGE_DOWN,          LABEL_PAGE_DOWN},
      {ICON_BACKGROUND,         LABEL_BACKGROUND},
      {ICON_BACKGROUND,         LABEL_BACKGROUND},
      {ICON_APPLY,              LABEL_SAVE},
      {ICON_BACK,               LABEL_BACK},
    }
};

GUI_RECT gridMeshZvalueRect = {PROBING_POINT_X0,    PROBING_POINT_Y0,    PROBING_POINT_X1,    PROBING_POINT_Y1};
GUI_RECT gridMeshBedRect    = {PROBING_POINT_X0-23, PROBING_POINT_Y0-23, PROBING_POINT_X1+23, PROBING_POINT_Y1+23};
GUI_RECT zvaluetextrect     = {
                                (PROBING_POINT_X1 + PROBING_POINT_X0)/2 - 40, PROBING_POINT_Y1 + 30 ,
                                (PROBING_POINT_X1 + PROBING_POINT_X0)/2 + 40, PROBING_POINT_Y1 + 30 + 24
                              };


// z calibration positions
uint8_t levelingPointPos[PROBING_POINT_X_COL*PROBING_POINT_Y_ROW][2] = {
  {25,25},
  {110,25},
  {195,25},

  {25,110},
  {110,110},
  {195,110},
  
  {25,195},
  {110,195},
  {195,195},
};

uint8_t jigzag_Point_XY[6][2] = {
  //          X               ,             Y              
  {-Z_CAL_PRINTING_LINE_WIDTH, -Z_CAL_PRINTING_LINE_WIDTH}, 
  {Z_CAL_PRINTING_LINE_WIDTH,  -Z_CAL_PRINTING_LINE_WIDTH}, 
  {Z_CAL_PRINTING_LINE_WIDTH,  0                         }, 
  {-Z_CAL_PRINTING_LINE_WIDTH, 0                         }, 
  {-Z_CAL_PRINTING_LINE_WIDTH, Z_CAL_PRINTING_LINE_WIDTH }, 
  {Z_CAL_PRINTING_LINE_WIDTH,  Z_CAL_PRINTING_LINE_WIDTH },
};

float jigzag_Point_E[6] = {
 // E
  0, 
  FRIST_LAYER_EXTRUDE_MM, 
  FRIST_LAYER_EXTRUDE_MM/2, 
  FRIST_LAYER_EXTRUDE_MM, 
  FRIST_LAYER_EXTRUDE_MM/2, 
  FRIST_LAYER_EXTRUDE_MM,
};

uint16_t hSpaceOfGrid, vSpaceOfGrid;
uint16_t doubleTouchCheck = 0;
uint16_t originIndexX, originIndexY;
float z_value_temp; // 임시 값 저장

uint32_t beginTime;
uint32_t doubleTouchGapTime = 500;

char  z_Cal_Printing_X_Pos = 0;
char  z_Cal_Printing_Y_Pos = 0;
float z_Cal_Printing_E_Pos = 0;

bool isFirstPrinting = true;
bool isNeedUpdateValue = true;

void drawGridMeshZValue(void)
{
    GUI_ClearPrect(&gridMeshBedRect);
    GUI_DrawPrect(&gridMeshZvalueRect);

    hSpaceOfGrid = (gridMeshZvalueRect.x1 - gridMeshZvalueRect.x0)/(addZOffsetCal.colN-1);
    vSpaceOfGrid = (gridMeshZvalueRect.y1 - gridMeshZvalueRect.y0)/(addZOffsetCal.rowN-1);
    uint16_t relativeR;

    //draw bule circle
    for (uint16_t i = 0; i < addZOffsetCal.colN; i++)
    {
      for (uint16_t j = 0; j < addZOffsetCal.rowN; j++)
      {
        if(addZOffsetCal.add_z_value[i][j] < 0)
        {
          GUI_SetColor(CREMAKERBLUE);
          relativeR = 15*(addZOffsetCal.add_z_value[i][j]/addZOffsetCal.min_add_z_value) + 5;

          GUI_FillCircle(gridMeshZvalueRect.x0 + hSpaceOfGrid * i, gridMeshZvalueRect.y0 + vSpaceOfGrid * j, relativeR);
        }        
      }
    }

    GUI_SetColor(WHITE);
    //draw grid
    for(uint16_t i = 0; i<(addZOffsetCal.colN-1);i++)
    {
        GUI_VLine(gridMeshZvalueRect.x0 + hSpaceOfGrid * i, gridMeshZvalueRect.y0, gridMeshZvalueRect.y1);
    }
    for(uint16_t i = 0; i<(addZOffsetCal.rowN-1);i++)
    {
        GUI_HLine(gridMeshZvalueRect.x0, gridMeshZvalueRect.y0  + vSpaceOfGrid * i, gridMeshZvalueRect.x1);
    }       

    //draw yellow circle
    for (uint16_t i = 0; i < addZOffsetCal.colN; i++)
    {
      for (uint16_t j = 0; j < addZOffsetCal.rowN; j++)
      {
        GUI_SetColor(CREMAKERYELLOW);
        relativeR = 5;

        GUI_FillCircle(gridMeshZvalueRect.x0 + hSpaceOfGrid * i, gridMeshZvalueRect.y0 + vSpaceOfGrid * j, relativeR);
      }
    }

    //draw orange circle
    for (uint16_t i = 0; i < addZOffsetCal.colN; i++)
    {
      for (uint16_t j = 0; j < addZOffsetCal.rowN; j++)
      {
        if(addZOffsetCal.add_z_value[i][j] > 0)
        {
          GUI_SetColor(CREMAKERORANGE);
          relativeR = 15*(addZOffsetCal.add_z_value[i][j]/addZOffsetCal.max_add_z_value) + 5;

          GUI_FillCircle(gridMeshZvalueRect.x0 + hSpaceOfGrid * i, gridMeshZvalueRect.y0 + vSpaceOfGrid * j, relativeR);
        }        
      }
    }

    GUI_SetColor(WHITE);
}

void redrawZvalue()
{
  char tempstr[10];
  GUI_ClearPrect(&zvaluetextrect);  
  sprintf(tempstr, "%.3f", z_value_temp);
  GUI_DispStringCenter((PROBING_POINT_X1 + PROBING_POINT_X0) / 2, PROBING_POINT_Y1 + 30, (uint8_t *)tempstr);
}

void drawCircleAtPoint()
{ 
  if(isNeedUpdateValue)
  {
    z_value_temp = addZOffsetCal.add_z_value[addZOffsetCal.colindex][addZOffsetCal.rowindex];
  }
  drawGridMeshZValue();
  redrawZvalue();
  GUI_DrawCircle(gridMeshZvalueRect.x0 + hSpaceOfGrid * addZOffsetCal.colindex, 
                 gridMeshZvalueRect.y0 + vSpaceOfGrid * addZOffsetCal.rowindex, 22);
}

void menuAddZOffsetCal(void)
{
  KEY_VALUES key_num = KEY_IDLE;

  originIndexY = addZOffsetCal.rowindex = addZOffsetCal.rowN-1;
  originIndexX = addZOffsetCal.colindex = 0;

  addZOffsetCal.posindex = 0;
  addZOffsetCal.printingindex = 0;

  isFirstPrinting = true;

  menuDrawPage(&addZOffsetCalItems);
  drawGridMeshZValue();

  while (infoMenu.menu[infoMenu.cur] == menuAddZOffsetCal)
  {
    key_num = menuKeyGetValue();

    switch (key_num)
    {
      case KEY_ICON_2:
        if(isFirstPrinting)
        {
          storeCmd("G28\n");
          storeCmd("G29\n");     
          storeCmd("M109 S200\n");    
          storeCmd("M190 S60\n");
          
          isFirstPrinting = false;
        }

        for(addZOffsetCal.printingindex = 0; addZOffsetCal.printingindex < sizeof(jigzag_Point_XY)/sizeof(jigzag_Point_XY[0]); addZOffsetCal.printingindex++)
        {
          z_Cal_Printing_X_Pos = levelingPointPos[addZOffsetCal.posindex][X_AXIS] + jigzag_Point_XY[addZOffsetCal.printingindex][X_AXIS];
          z_Cal_Printing_Y_Pos = levelingPointPos[addZOffsetCal.posindex][Y_AXIS] + jigzag_Point_XY[addZOffsetCal.printingindex][Y_AXIS];
          z_Cal_Printing_E_Pos = jigzag_Point_E[addZOffsetCal.printingindex];

          if(addZOffsetCal.printingindex==0)
          {
            storeCmd("M83\n"); // relative E positioning mode
            storeCmd("G0 X%d Y%d Z0.3 F%d\n",z_Cal_Printing_X_Pos, z_Cal_Printing_Y_Pos, FRIST_LAYER_MOVE_SPEED);
            storeCmd("G1 E1.2 F1000\n");
          }
          else
          {
            storeCmd("G1 X%d Y%d E%.3f F%d\n",z_Cal_Printing_X_Pos, z_Cal_Printing_Y_Pos, z_Cal_Printing_E_Pos, FRIST_LAYER_PRINTING_SPEED);
          }
        }

        storeCmd("G91\n"); // relative positioning mode
        storeCmd("G1 E-1\n");
        if(addZOffsetCal.colindex == (addZOffsetCal.colN-1))
        {
          if(addZOffsetCal.rowindex == 0)
          {
            storeCmd("G0 X-70 Y10 Z5 F4000\n");
          }
          else
          {
            storeCmd("G0 X-70 Y40 Z5 F4000\n");
          }
        }
        else if(addZOffsetCal.rowindex == 0)
        {
          storeCmd("G0 X40 Y10 Z5 F4000\n");
        }
        else
        {
          storeCmd("G0 X40 Y40 Z5 F4000\n");
        }
        storeCmd("G90\n"); // absolute positioning mode
        
        break;

      case KEY_ICON_3:
        addZOffsetCal.colindex++;        
        if(addZOffsetCal.colindex > (addZOffsetCal.colN-1))
        {
            addZOffsetCal.colindex = 0;
            addZOffsetCal.rowindex--;
            if(addZOffsetCal.rowindex < 0)
            {
                addZOffsetCal.rowindex = addZOffsetCal.rowN-1;
            }
        }

        addZOffsetCal.posindex++;
        if(addZOffsetCal.posindex > (addZOffsetCal.colN)*(addZOffsetCal.rowN)-1)
          addZOffsetCal.posindex = 0;
        drawCircleAtPoint();
        
        break;

      case KEY_ICON_6:
        if(addZOffsetCal.add_z_value[addZOffsetCal.colindex][addZOffsetCal.rowindex] != z_value_temp)
        {
          originIndexX = addZOffsetCal.colindex;
          originIndexY = addZOffsetCal.rowindex;
          storeCmd("M101 I%d J%d M%.3f A\n",addZOffsetCal.colindex, (addZOffsetCal.rowN-1) - addZOffsetCal.rowindex, z_value_temp);
          isNeedUpdateValue = false;
        }
        else if((OS_GetTimeMs() - beginTime) < doubleTouchGapTime)
        {
          doubleTouchCheck++;
          if(doubleTouchCheck > 3)
          {
            storeCmd("M101 R\n");
            doubleTouchCheck = 0;
            originIndexY = addZOffsetCal.rowindex = addZOffsetCal.rowN-1;
            originIndexX = addZOffsetCal.colindex = 0;
            addZOffsetCal.posindex = 0;
            addZOffsetCal.printingindex = 0;
          }
          beginTime = OS_GetTimeMs();
        }
        else
        {
          doubleTouchCheck = 1;
          beginTime = OS_GetTimeMs();
        }

        break;

      case KEY_ICON_7:        
        if (infoMachineSettings.EEPROM == 1)
        {
          setDialogText(addZOffsetCalItems.title.index, LABEL_SAVE_VALUE_AFTER_TEST, LABEL_YES, LABEL_NO);
          showDialog(DIALOG_TYPE_QUESTION, saveEepromSettings, NULL, NULL);
        }
        storeCmd("M84\n");
        infoMenu.cur--;
        break;

      default:
        #if LCD_ENCODER_SUPPORT
          if (encoderPosition)
          {
            z_value_temp += encoderPosition * 0.01;
            redrawZvalue();
            encoderPosition = 0;
          }
        #endif
        break;
    }

    loopProcess();

    if(addZOffsetCal.isNeedRedraw)
    {       
      addZOffsetCal.colindex = originIndexX;
      addZOffsetCal.rowindex = originIndexY;
      addZOffsetCal.isNeedRedraw = false;
      isNeedUpdateValue = true;
      drawCircleAtPoint();
    }    
  }
}
