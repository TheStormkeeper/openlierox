/*
 *  PrecompiledHeader.h
 *  OpenLieroX
 *
 *	Use this header as your precompiled header if your compiler supports that.
 *
 *  Created by Albert Zeyer on 09.02.09.
 *  code under LGPL
 *
 */

#ifdef __PRECOMPILEDHEADER_H__
#error PrecompiledHeader.hpp cannot be used in the regular code.
#define __PRECOMPILEDHEADER_H__
#endif

#if !defined(__cplusplus)
// We're compiling C-code here, HawkNL most probably
#else

#include "AuxLib.h"
#include "Cache.h"
#include "CBanList.h"
#include "CBonus.h"
#include "CBytestream.h"
#include "CChannel.h"
#include "CChatBox.h"
#include "CClient.h"
#include "CClientNetEngine.h"
#include "CFont.h"
#include "CGameMode.h"
#include "CGameScript.h"
#include "CGameSkin.h"
#include "ChatCommand.h"
#include "CHideAndSeek.h"
#include "CInput.h"
#include "Clipboard.h"
#include "CMap.h"
#include "CNinjaRope.h"
#include "Color.h"
#include "Command.h"
#include "ConfigHandler.h"
#include "console.h"
#include "Consts.h"
#include "ConversationLogger.h"
#include "CProjectile.h"
#include "CrashHandler.h"
#include "CScriptableVars.h"
#include "CServerConnection.h"
#include "CServer.h"
#include "CServerNetEngine.h"
#include "CShootList.h"
#include "CssParser.h"
#include "CsvReader.h"
#include "Cursor.h"
#include "CVec.h"
#include "CViewport.h"
#include "CWeather.h"
#include "CWormBot.h"
#include "CWorm.h"
#include "CWormHuman.h"
#include "CWpnRest.h"
#include "Debug.h"
#include "DedicatedControl.h"
#include "EndianSwap.h"
#include "Entity.h"
#include "Error.h"
#include "Event.h"
#include "EventQueue.h"
#include "FastVector.h"
#include "FeatureList.h"
#include "FileDownload.h"
#include "FindFile.h"
#include "FlagInfo.h"
#include "FontEffects.h"
#include "FontHandling.h"
#include "Frame.h"
#include "Functors.h"
#include "GfxPrimitives.h"
#include "GuiPrimitives.h"
#include "HTTP.h"
#include "IniReader.h"
#include "InputEvents.h"
#include "InternDataClass.h"
#include "IpToCountryDB.h"
#include "IRC.h"
#include "Iterator.h"
#include "libirc_rfcnumeric.h"
#include "LieroX.h"
#include "MathLib.h"
#include "Music.h"
#include "Mutex.h"
#include "Networking.h"
#include "NewNetEngine.h"
#include "NotifyUser.h"
#include "OLXG15.h"
#include "Options.h"
#include "Physics.h"
#include "PhysicsLX56.h"
#include "PixelFunctors.h"
#include "Process.h"
#include "ProfileSystem.h"
#include "Protocol.h"
#include "RandomNumberList.h"
#include "ReadWriteLock.h"
#include "Ref.h"
#include "RemoteHost.h"
#include "sex.h"
#include "SmartPointer.h"
#include "Sounds.h"
#include "StackWalker.h"
#include "StringBuf.h"
#include "StringUtils.h"
#include "StyleVar.h"
#include "TaskManager.h"
#include "ThreadPool.h"
#include "ThreadVar.h"
#include "Timer.h"
#include "TSVar.h"
#include "types.h"
#include "Unicode.h"
#include "UploadSpeedTest.h"
#include "Utils.h"
#include "Version.h"
#include "XMLutils.h"
#include "DeprecatedGUI/CAnimation.h"
#include "DeprecatedGUI/CBar.h"
#include "DeprecatedGUI/CBox.h"
#include "DeprecatedGUI/CBrowser.h"
#include "DeprecatedGUI/CButton.h"
#include "DeprecatedGUI/CCheckbox.h"
#include "DeprecatedGUI/CCombobox.h"
#include "DeprecatedGUI/CGuiLayout.h"
#include "DeprecatedGUI/CGuiSkin.h"
#include "DeprecatedGUI/CGuiSkinnedLayout.h"
#include "DeprecatedGUI/CImage.h"
#include "DeprecatedGUI/CInputBox.h"
#include "DeprecatedGUI/CLabel.h"
#include "DeprecatedGUI/CLine.h"
#include "DeprecatedGUI/CListview.h"
#include "DeprecatedGUI/CMenu.h"
#include "DeprecatedGUI/CProgressbar.h"
#include "DeprecatedGUI/CScrollbar.h"
#include "DeprecatedGUI/CSlider.h"
#include "DeprecatedGUI/CTextbox.h"
#include "DeprecatedGUI/CTextButton.h"
#include "DeprecatedGUI/CTitleButton.h"
#include "DeprecatedGUI/CWidget.h"
#include "DeprecatedGUI/CWidgetList.h"
#include "DeprecatedGUI/Graphics.h"
#include "DeprecatedGUI/Menu.h"
#endif // !defined(__cplusplus)
