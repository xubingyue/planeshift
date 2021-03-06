/*
 * pawsdndbutton.cpp - Author: Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
// pawsdndbutton.cpp: implementation of the pawsDnDButton class.
//
//////////////////////////////////////////////////////////////////////

#include <csutil/event.h>

#include <psconfig.h>
#include <ivideo/fontserv.h>
#include <iutil/evdefs.h>
#include <iutil/plugin.h>

#include <isoundmngr.h>

#include <iscenemanipulate.h>
#include "globals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "gui/pawsdndbutton.h"
#include "gui/pawsscrollmenu.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawsprefmanager.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsDnDButton::pawsDnDButton() :
    pawsButton(),
    mgr(NULL),
    dragDrop(1),
    dragDropInProgress(0),
    action(NULL),
    containerID(0),
    indexBase(0),
    editMode(0),
    Callback(NULL),
    ImageNameCallback(NULL),
    NameCallback(NULL),
    ActionCallback(NULL),
    warnLevel(0.0),
    warnLow(false),
    warnMode(0),
    dangerLevel(0.0),
    dangerLow(false),
    dangerMode(0),
    flashLevel(0.0),
    flashLow(false),
    flashMode(0),
    DnDLock(false),
    spellProgress(NULL),
    lastToolTipTime(999999999)
{
    factory = "pawsDnDButton";
}

bool pawsDnDButton::Setup(iDocumentNode* node)
{
    // Check for toggle
    toggle = node->GetAttributeValueAsBool("toggle", true);

    // Check for keyboard shortcut for this button
    const char* key = node->GetAttributeValue("key");
    if(key)
    {
        if(!strcasecmp(key,"Enter"))
            keybinding = 10;
        else
            keybinding = *key;
    }
    // Check for sound to be associated to Buttondown
    csRef<iDocumentAttribute> soundAttribute = node->GetAttribute("sound");
    if(soundAttribute)
    {
        csString soundName = node->GetAttributeValue("sound");
        SetSound(soundName);
    }
    else
    {
        csString name2;

        csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
        if(buttonLabelNode)
            name2 = buttonLabelNode->GetAttributeValue("text");

        name2.Downcase();

        if(name2 == "ok")
            SetSound("gui.ok");
        else if(name2 == "quit")
            SetSound("gui.quit");
        else if(name2 == "cancel")
            SetSound("gui.cancel");
        else
            SetSound("sound.standardButtonClick");
    }

    // Check for notify widget
    csRef<iDocumentAttribute> notifyAttribute = node->GetAttribute("notify");
    if(notifyAttribute)
        notify = PawsManager::GetSingleton().FindWidget(notifyAttribute->GetValue());

    // Check for mouse over
    //changeOnMouseOver = node->GetAttributeValueAsBool("changeonmouseover", false);

    // Get the down button image name.
    csRef<iDocumentNode> buttonDownImage = node->GetNode("buttondown");
    if(buttonDownImage)
    {
        csString downImageName = buttonDownImage->GetAttributeValue("resource");
        SetDownImage(downImageName);
        downTextOffsetX = buttonDownImage->GetAttributeValueAsInt("textoffsetx");
        downTextOffsetY = buttonDownImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonUpImage = node->GetNode("buttonup");
    if(buttonUpImage)
    {
        csString upImageName = buttonUpImage->GetAttributeValue("resource");
        SetUpImage(upImageName);
        upTextOffsetX = buttonUpImage->GetAttributeValueAsInt("textoffsetx");
        upTextOffsetY = buttonUpImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the down button image name.
    csRef<iDocumentNode> buttonGreyDownImage = node->GetNode("buttongraydown");
    if(buttonGreyDownImage)
    {
        csString greyDownImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyDownImageName);
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonGreyUpImage = node->GetNode("buttongrayup");
    if(buttonGreyUpImage)
    {
        csString greyUpImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyUpImageName);
    }

    // Get the "on char name flash" button image name.
    csRef<iDocumentNode> buttonSpecialImage = node->GetNode("buttonspecial");
    if(buttonSpecialImage)
    {
        csString onSpecialImageName = buttonSpecialImage->GetAttributeValue("resource");
        SetOnSpecialImage(onSpecialImageName);
    }


    // Get the button label
    csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
    if(buttonLabelNode)
    {
        buttonLabel = PawsManager::GetSingleton().Translate(buttonLabelNode->GetAttributeValue("text"));
    }
 
    backgroundBackup=GetBackground();

    originalFontColour = GetFontColour();

    return true;
}

void pawsDnDButton::Draw()
{

    if(spellProgress!=NULL )
    {
        csTicks currentTime     = csGetTicks();
        float   elapsedTime     = (float)currentTime-(float)startTime;
        float   currentProgress = elapsedTime/(float)castingTime;

        if (currentProgress >= 1.0)
        {
            castingTime = 0;
            spellProgress->Hide(); //this *shouldn't* be necessary as the server should be pushing out an updated spell list about this time, but we include this in case of missed packets.
            pawsButton::Draw();

            return;
        }

        //update the progress meter
        spellProgress->SetCurrentValue(currentProgress);
        spellProgress->SetRelativeFrame( 1, 1, screenFrame.Width()-2, screenFrame.Height()-2 );

        csTicks remainingTime   = (castingTime-(currentTime-startTime))/1000;
        if( lastToolTipTime > remainingTime ) //only update once per second
        {
            if( remainingTime>59 )
            {
                unsigned int hours = remainingTime/3600;
                unsigned int minutes = (remainingTime-(hours*3600))/60;
                unsigned int seconds = (remainingTime-(hours*3600)-(minutes*60));

                if( hours>0 )
                {
                    pawsWidget::FormatToolTip( "%s, %u hrs, %u mins, %u secs", baseToolTip.GetData(), hours, minutes, seconds );
                    spellProgress->FormatToolTip( "%s, %u hrs, %u mins, %u secs", baseToolTip.GetData(), hours, minutes, seconds );
                }
                else
                {
                    pawsWidget::FormatToolTip( "%s, %u mins, %u secs", baseToolTip.GetData(), minutes, seconds );
                    spellProgress->FormatToolTip( "%s, %u mins, %u secs", baseToolTip.GetData(), minutes, seconds );
                }
            }
            else
            {
                pawsWidget::FormatToolTip( "%s, %u seconds", baseToolTip.GetData(), remainingTime );
                spellProgress->FormatToolTip( "%s, %u seconds", baseToolTip.GetData(), remainingTime );
            }
            lastToolTipTime = remainingTime;
        }
    }

    pawsButton::Draw();
}

/**
 * Start the timer on a button. primarily used for active magic bar.
 *
 * startTicks is the time the server recognized the spell cast begin
 * currentTicks is the server's current time
 * duration is how long the spell lasts
 **/
void pawsDnDButton::Start(csTicks startTicks, csTicks currentTicks, csTicks duration)
{
    if( spellProgress!=NULL || duration==0 ) //if one already exists don't create a new one; if duration is 0 don't create one at all.
    {
        return;
    }

    spellProgress=new pawsProgressBar();
    AddChild(spellProgress);

    spellProgress->SetTotalValue(1.0);
    spellProgress->SetColor( 0, 0, 200 );
    spellProgress->SetWarningLevel(warnLevel, warnLow);
    spellProgress->SetDangerLevel(dangerLevel, warnLow);
    spellProgress->SetFlashLevel(flashLevel, warnLow);
    spellProgress->SetFlashRate( 250);

    float currentProgress = ((float)(currentTicks-startTicks))/(float)duration;
    spellProgress->SetCurrentValue(currentProgress);
    startTime = csGetTicks()-(currentTicks-startTicks);//note: this must be in terms of the client's ticks, not the server's
    this->castingTime = duration;
}


bool pawsDnDButton::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
    }

    if(node->GetAttributeValue("down"))
    {
        SetState(strcmp(node->GetAttributeValue("down"),"true")==0);
    }
    SetDnDLock(true);

    return true;
}

pawsDnDButton::~pawsDnDButton()
{
}

bool pawsDnDButton::OnMouseDown(int button, int modifiers, int x, int y)
{
#if 0
    bool empty;

    if(GetMaskingImage())
    {
        empty = false;
    }
    else if(GetText() && (*GetText())!=0)
    {
        empty = false;
    }
    else
    {
        empty = true;
    }
#endif

    if(!enabled)
        return false;

    if(button==csmbLeft && (dragDrop || psengine->GetSlotManager()->IsDragging()))
    {
        if(!GetDnDLock())
        {
            psengine->GetSlotManager()->CancelDrag();
            return pawsButton::OnMouseDown(button, modifiers, x, y);
        }
        else
        {
            if(!mgr)
            {
                mgr = psengine->GetSlotManager();
            }
            if(!mgr)
            {
                return false;
            }
            mgr->Handle(this);

            dragDropInProgress = true;
            return true;
        }
    }
    else
    {
        return pawsButton::OnMouseDown(button, modifiers, x, y);
    }
}

bool pawsDnDButton::CheckKeyHandled(int keyCode)
{
    if(keybinding && keyCode == keybinding)
    {
        OnMouseUp(-1,0,0,0);
        return true;
    }
    return false;
}

bool pawsDnDButton::OnMouseUp(int button, int modifiers, int x, int y)
{
    if(!enabled)
    {
        return false;
    }

    if(!toggle)
        SetState(false, false);

    if(button != -1)   // triggered by keyboard
    {
        // Check to make sure mouse is still in this button
        if(!Contains(x,y))
        {
            Debug1(LOG_PAWS, 0, "Not still in button so not pressed.");
            return true;
        }
    }

    if(notify != NULL)
    {
        notify->OnButtonReleased(button, modifiers, this);
    }
    else if(parent)
    {
        return parent->OnButtonReleased(button, modifiers, this);
    }

    return false;
}

iPawsImage* pawsDnDButton::GetMaskingImage()
{
    return maskImage;
}

bool pawsDnDButton::PlaceItem(const char* imageName, const char* Name, const char* toolTip, const char* action)
{
    bool validMove = false; //if function returns false don't delet the original

    if(imageName)
    {
        SetMaskingImage(imageName);
        if(ImageNameCallback)
        {
            if(ImageNameCallback->Get(id-indexBase))
            {
                ImageNameCallback->Get(id-indexBase).Replace(imageName);
            }
        }
        validMove = true;
    }
    else
    {
        ClearMaskingImage();
        //ImageNameCallback=NULL;
    }
    if(Name)
    {
        SetName(Name);
        if(NameCallback)
        {
            NameCallback->Get(id-indexBase).Replace(Name);
        }
        validMove = true;
    }
    else
    {
        if(toolTip)
        {
            SetName(toolTip);
            if(NameCallback)
            {
                NameCallback->Get(id-indexBase).Replace(toolTip);
            }
            validMove = true;
        }
        else
        {
            SetName("");
            //NameCallback=NULL;
        }
    }
    if(toolTip)
    {
        SetToolTip(toolTip);
    }
    if(action)
    {
        SetAction(action);
        if(ActionCallback)
        {
            ActionCallback->Get(id-indexBase).Replace(action);
        }
    }
    else
    {
        action=NULL;
        //ActionCallback=NULL;
    }
    if(!validMove)
    {
        return false;
    }
    return true;
}



void pawsDnDButton::SetMaskingImage(const char* image)
{
    if(ImageNameCallback)
    {
        ImageNameCallback->Get(id-indexBase).Replace(image);
    }
    pawsWidget::SetMaskingImage(image);
}

void pawsDnDButton::Clear()
{
    if(GetMaskingImage())
    {
        ClearMaskingImage();
    }
    SetToolTip("");
    baseToolTip.Empty();
    SetText("");

    if(ImageNameCallback)
    {
        ImageNameCallback->Get(id-indexBase).Clear();
    }
    if(NameCallback)
    {
        NameCallback->Get(id-indexBase).Clear();
    }
    if(ActionCallback)
    {
        ActionCallback->Get(id-indexBase).Clear();
    }
}

void pawsDnDButton::SetDragDropInProgress(int val)
{
    dragDropInProgress = val;
}

int pawsDnDButton::IsDragDropInProgress()
{
    return dragDropInProgress;
}

void pawsDnDButton::MouseOver(bool value)
{
}


void pawsDnDButton::DrawMask()
{
    // Draw the masking image
    int drawAlpha = -1;
    if (fade && parent && parent->GetMaxAlpha() >= 0 && bgImage && maskImage)
    {
        fadeVal = parent->GetFadeVal();
        alpha = parent->GetMaxAlpha();
        alphaMin = parent->GetMinAlpha();
        drawAlpha = (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010);
    }
    if (maskImage)
    {
        int imageX;

        imageX = screenFrame.xmin+(screenFrame.Width()/2)-(screenFrame.Height()/2);

        ClipToParent(true);
        maskImage->Draw(imageX, screenFrame.ymin, screenFrame.Height(), screenFrame.Height(), drawAlpha);
    }
}

void pawsDnDButton::EnableBackground( bool mode )
{
    if( GetBackground()!="" && GetBackground()!=NULL )
    {
        backgroundBackup=GetBackground();
    }
    if( mode==true )
    {
        if(backgroundBackup)
        {
            SetBackground(backgroundBackup);
        }
    }
    else //mode == false
    {
        SetBackground("");
    }
}

void pawsDnDButton::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    spellProgress->SetCurrentValue(value.GetFloat());
}

void pawsDnDButton::SetToolTip( const char* toolTip )
{
    baseToolTip = toolTip;
    pawsButton::SetToolTip( toolTip );
}

void pawsDnDButton::SetRelativeFrame(int x, int y, int width, int height)
{
    if( spellProgress )
    {
        spellProgress->SetRelativeFrame(x, y, width, height);
    }
    pawsWidget::SetRelativeFrame(x, y, width, height);
}

void pawsDnDButton::SetWarnLevel( float val, bool low )
{
    warnLevel = val;
    warnLow   = low;
}
void pawsDnDButton::SetDangerLevel( float val, bool low )
{
    dangerLevel = val;    
    dangerLow   = low;
}
void pawsDnDButton::SetFlashLevel( float val, bool low )
{
    flashLevel = val;    
    flashLow   = low;
}
void pawsDnDButton::SetWarnMode( int i )
{
    warnMode   = i;
}
void pawsDnDButton::SetDangerMode( int i )
{
    dangerMode   = i;
}
void pawsDnDButton::SetFlashMode( int i )
{
    flashMode   = i;
}
