/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_EDITOR_EDITOR_H
#define GAME_EDITOR_EDITOR_H

#include <math.h>

#include <base/math.h>
#include <base/system.h>

#include <base/tl/algorithm.h>
#include <base/tl/array.h>
#include <base/tl/sorted_array.h>
#include <base/tl/string.h>

#include <game/client/ui.h>
#include <game/mapitems.h>
#include <game/client/render.h>

#include <engine/shared/config.h>
#include <engine/shared/datafile.h>
#include <engine/editor.h>
#include <engine/graphics.h>

#include "auto_map.h"

typedef void (*INDEX_MODIFY_FUNC)(int *pIndex);

enum
{
    OP_EDITOR_NONE=0,
    OP_EDITOR_BRUSH_GRAB,
    OP_EDITOR_BRUSH_DRAW,
    OP_EDITOR_BRUSH_PAINT,
    OP_EDITOR_PAN_WORLD,
    OP_EDITOR_PAN_EDITOR,
};

//CRenderTools m_RenderTools;

// CEditor SPECIFIC
enum
{
	MODE_LAYERS=0,
	MODE_IMAGES,

	DIALOG_NONE=0,
	DIALOG_FILE,
};

struct CEntity
{
	CPoint m_Position;
	int m_Type;
};

class CEnvelope
{
public:
	int m_Channels;
	array<CEnvPoint> m_lPoints;
	char m_aName[32];
	float m_Bottom, m_Top;
	bool m_Synchronized;

	CEnvelope(int Chan)
	{
		m_Channels = Chan;
		m_aName[0] = 0;
		m_Bottom = 0;
		m_Top = 0;
		m_Synchronized = true;
	}

	void Resort()
	{
		sort(m_lPoints.all());
		FindTopBottom(0xf);
	}

	void FindTopBottom(int ChannelMask)
	{
		m_Top = -1000000000.0f;
		m_Bottom = 1000000000.0f;
		for(int i = 0; i < m_lPoints.size(); i++)
		{
			for(int c = 0; c < m_Channels; c++)
			{
				if(ChannelMask&(1<<c))
				{
					float v = fx2f(m_lPoints[i].m_aValues[c]);
					if(v > m_Top) m_Top = v;
					if(v < m_Bottom) m_Bottom = v;
				}
			}
		}
	}

	int Eval(float Time, float *pResult)
	{
		CRenderTools::RenderEvalEnvelope(m_lPoints.base_ptr(), m_lPoints.size(), m_Channels, Time, pResult);
		return m_Channels;
	}

	void AddPoint(int Time, int v0, int v1=0, int v2=0, int v3=0)
	{
		CEnvPoint p;
		p.m_Time = Time;
		p.m_aValues[0] = v0;
		p.m_aValues[1] = v1;
		p.m_aValues[2] = v2;
		p.m_aValues[3] = v3;
		p.m_Curvetype = CURVETYPE_LINEAR;
		m_lPoints.add(p);
		Resort();
	}

	float EndTime()
	{
		if(m_lPoints.size())
			return m_lPoints[m_lPoints.size()-1].m_Time*(1.0f/1000.0f);
		return 0;
	}
};


class CLayer;
class CLayerGroup;
class CEditorMap;

class CLayer
{
public:
	class CEditor *m_pEditor;
	class IGraphics *Graphics();
	class ITextRender *TextRender();

	CLayer()
	{
		m_Type = LAYERTYPE_INVALID;
		str_copy(m_aName, "(invalid)", sizeof(m_aName));
		m_Visible = true;
		m_Readonly = false;
		m_SaveToMap = true;
		m_Flags = 0;
		m_pEditor = 0;
	}

	virtual ~CLayer()
	{
	}


	virtual void BrushSelecting(CUIRect Rect) {}
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect) { return 0; }
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect) {}
	virtual void BrushDraw(CLayer *pBrush, float x, float y) {}
	virtual void BrushPlace(CLayer *pBrush, float x, float y) {}
	virtual void BrushFlipX() {}
	virtual void BrushFlipY() {}
	virtual void BrushRotate(float Amount) {}

	virtual void Render() {}
	virtual int RenderProperties(CUIRect *pToolbox) { return 0; }

	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc) {}
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc) {}

	virtual void GetSize(float *w, float *h) { *w = 0; *h = 0;}

	char m_aName[12];
	int m_Type;
	int m_Flags;

	bool m_Readonly;
	bool m_Visible;
	bool m_SaveToMap;
};

class CLayerGroup
{
public:
	class CEditorMap *m_pMap;

	array<CLayer*> m_lLayers;

	int m_OffsetX;
	int m_OffsetY;

	int m_ParallaxX;
	int m_ParallaxY;

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	char m_aName[12];
	bool m_GameGroup;
	bool m_Visible;
	bool m_SaveToMap;
	bool m_Collapse;

	CLayerGroup();
	~CLayerGroup();

	void Convert(CUIRect *pRect);
	void Render();
	void MapScreen();
	void Mapping(float *pPoints);

	void GetSize(float *w, float *h);

	void DeleteLayer(int Index);
	int SwapLayers(int Index0, int Index1);

	bool IsEmpty() const
	{
		return m_lLayers.size() == 0;
	}

	void Clear()
	{
		m_lLayers.delete_all();
	}

	void AddLayer(CLayer *l);

	void ModifyImageIndex(INDEX_MODIFY_FUNC Func)
	{
		for(int i = 0; i < m_lLayers.size(); i++)
			m_lLayers[i]->ModifyImageIndex(Func);
	}

	void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC Func)
	{
		for(int i = 0; i < m_lLayers.size(); i++)
			m_lLayers[i]->ModifyEnvelopeIndex(Func);
	}
};

class CEditorImage : public CImageInfo
{
public:
	CEditor *m_pEditor;

	CEditorImage(CEditor *pEditor)
	: m_AutoMapper(pEditor)
	{
		m_pEditor = pEditor;
		m_TexID = -1;
		m_aName[0] = 0;
		m_External = 0;
		m_Width = 0;
		m_Height = 0;
		m_pData = 0;
		m_Format = 0;
	}

	~CEditorImage();

	void AnalyseTileFlags();

	int m_TexID;
	int m_External;
	char m_aName[128];
	unsigned char m_aTileFlags[256];
	class CAutoMapper m_AutoMapper;
};

class CEditorMap
{
	void MakeGameGroup(CLayerGroup *pGroup);
	void MakeGameLayer(CLayer *pLayer);
public:
	CEditor *m_pEditor;
	bool m_Modified;

	CEditorMap()
	{
		Clean();
	}

	array<CLayerGroup*> m_lGroups;
	array<CEditorImage*> m_lImages;
	array<CEnvelope*> m_lEnvelopes;

	class CMapInfo
	{
	public:
		char m_aAuthorTmp[32];
		char m_aVersionTmp[16];
		char m_aCreditsTmp[128];
		char m_aLicenseTmp[32];

		char m_aAuthor[32];
		char m_aVersion[16];
		char m_aCredits[128];
		char m_aLicense[32];

		void Reset()
		{
			m_aAuthorTmp[0] = 0;
			m_aVersionTmp[0] = 0;
			m_aCreditsTmp[0] = 0;
			m_aLicenseTmp[0] = 0;

			m_aAuthor[0] = 0;
			m_aVersion[0] = 0;
			m_aCredits[0] = 0;
			m_aLicense[0] = 0;
		}
	};
	CMapInfo m_MapInfo;

	class CLayerGame *m_pGameLayer;
	CLayerGroup *m_pGameGroup;

	CEnvelope *NewEnvelope(int Channels)
	{
		m_Modified = true;
		CEnvelope *e = new CEnvelope(Channels);
		m_lEnvelopes.add(e);
		return e;
	}

	void DeleteEnvelope(int Index);

	CLayerGroup *NewGroup()
	{
		m_Modified = true;
		CLayerGroup *g = new CLayerGroup;
		g->m_pMap = this;
		m_lGroups.add(g);
		return g;
	}

	int SwapGroups(int Index0, int Index1)
	{
		if(Index0 < 0 || Index0 >= m_lGroups.size()) return Index0;
		if(Index1 < 0 || Index1 >= m_lGroups.size()) return Index0;
		if(Index0 == Index1) return Index0;
		m_Modified = true;
		swap(m_lGroups[Index0], m_lGroups[Index1]);
		return Index1;
	}

	void DeleteGroup(int Index)
	{
		if(Index < 0 || Index >= m_lGroups.size()) return;
		m_Modified = true;
		delete m_lGroups[Index];
		m_lGroups.remove_index(Index);
	}

	void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc)
	{
		m_Modified = true;
		for(int i = 0; i < m_lGroups.size(); i++)
			m_lGroups[i]->ModifyImageIndex(pfnFunc);
	}

	void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc)
	{
		m_Modified = true;
		for(int i = 0; i < m_lGroups.size(); i++)
			m_lGroups[i]->ModifyEnvelopeIndex(pfnFunc);
	}

	void Clean();
	void CreateDefault(int EntitiesTexture);

	// io
	int Save(class IStorage *pStorage, const char *pFilename);
	int Load(class IStorage *pStorage, const char *pFilename, int StorageType);
};


struct CProperty
{
	const char *m_pName;
	int m_Value;
	int m_Type;
	int m_Min;
	int m_Max;
};

enum
{
	PROPTYPE_NULL=0,
	PROPTYPE_BOOL,
	PROPTYPE_INT_STEP,
	PROPTYPE_INT_SCROLL,
	PROPTYPE_COLOR,
	PROPTYPE_IMAGE,
	PROPTYPE_ENVELOPE,
	PROPTYPE_SHIFT,
};

typedef struct
{
	int x, y;
	int w, h;
} RECTi;

class CLayerTiles : public CLayer
{
public:
	CLayerTiles(int w, int h);
	~CLayerTiles();

	void Resize(int NewW, int NewH);
	void Shift(int Direction);

	void MakePalette();
	virtual void Render();

	int ConvertX(float x) const;
	int ConvertY(float y) const;
	void Convert(CUIRect Rect, RECTi *pOut);
	void Snap(CUIRect *pRect);
	void Clamp(RECTi *pRect);

	virtual void BrushSelecting(CUIRect Rect);
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect);
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
	virtual void BrushFlipX();
	virtual void BrushFlipY();
	virtual void BrushRotate(float Amount);

	virtual void ShowInfo();
	virtual int RenderProperties(CUIRect *pToolbox);

	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc);
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc);

	void PrepareForSave();

	void GetSize(float *w, float *h) { *w = m_Width*32.0f; *h = m_Height*32.0f; }

	int m_TexID;
	int m_Game;
	int m_Image;
	int m_Width;
	int m_Height;
	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;
	CTile *m_pTiles;
};

class CLayerQuads : public CLayer
{
public:
	CLayerQuads();
	~CLayerQuads();

	virtual void Render();
	CQuad *NewQuad();

	virtual void BrushSelecting(CUIRect Rect);
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect);
	virtual void BrushPlace(CLayer *pBrush, float wx, float wy);
	virtual void BrushFlipX();
	virtual void BrushFlipY();
	virtual void BrushRotate(float Amount);

	virtual int RenderProperties(CUIRect *pToolbox);

	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc);
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc);

	void GetSize(float *w, float *h);

	int m_Image;
	array<CQuad> m_lQuads;
};

class CLayerGame : public CLayerTiles
{
public:
	CLayerGame(int w, int h);
	~CLayerGame();

	virtual int RenderProperties(CUIRect *pToolbox);
};

enum
{
    EDITOR_VIEW_NONE = 0,
    EDITOR_VIEW_GROUP,
    EDITOR_VIEW_MENU,
    EDITOR_VIEW_LAYER,
    EDITOR_VIEW_EDITOR,
    EDITOR_VIEW_SCRIPT,
    EDITOR_VIEW_CURVES,
    EDITOR_VIEW_NODES,
};

enum
{
    EDITOR_UI_ICON_NONE = -1,
    EDITOR_UI_ICON_LEX,
    EDITOR_UI_ICON_REX,
    EDITOR_UI_ICON_LAYER,
    EDITOR_UI_ICON_EDITOR,
    EDITOR_UI_ICON_SCRIPT,
    EDITOR_UI_ICON_CURVES,
    EDITOR_UI_ICON_NODES,
};

class CView
{
    int m_uLEx;
    int m_uREx;
    int m_uSelector;
    int m_uSelectorMenu;
    friend class CEditor;
    friend class CViewGroup;
protected:
    class CEditor *m_pEditor;

    class CViewGroup *m_pParent;
    float m_Width;
    float m_Height;
public:
    CView(class CEditor *pEditor, class CViewGroup *pParent);
    virtual int GetType() = 0;
    virtual CUIRect Render(CUIRect *pView);
    virtual float GetMinWidth() { return 100; }
    virtual float GetMaxWidth() { return 1000; }
    virtual float GetMinHeight() { return 1; }
    virtual float GetMaxHeight() { return 1000; }
    float DeltaWidth(float x);
    float DeltaHeight(float y);
    bool SetWidth(float x);
    bool SetHeight(float y);

    static int PopupSelector(CEditor *pView, CUIRect View, void *pUser);

    CUIRect GetView(CUIRect *pView, int m_Direction);
};

class CViewGroup : public CView
{
    friend class CView;
protected:
    array<CView *> m_lpChildren;
public:
    CViewGroup(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent) {}
    int GetType() { return EDITOR_VIEW_GROUP; }
    CUIRect Render(CUIRect *pView);
    void Replace(CView *pOldView, CView *pNewView);
};

class CViewMenu : public CView
{
    int m_uFile;
    int m_uViewSelector;
public:
    CViewMenu(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent)
    {
        m_Width = 0;
        m_Height = 16;
    }
    int GetType() { return EDITOR_VIEW_MENU; }
    float GetMinHeight() { return 16; }
    float GetMaxHeight() { return 16; }
    CUIRect Render(CUIRect *pView);
};

class CViewLayer : public CView
{
    int m_uTabButton;
    int m_Mode;

    //RenderLayers stuff
    int m_uLayerScrollBar;
    float m_LayerScrollValue;
    int m_uLayerNewGroupButton;
    int m_uLayerGroupPopupId;

    //RenderImages stuff
    int m_uImageScrollBar;
    float m_ImageScrollValue;
    int m_uImagePopupImageID;
    int m_uImageNewImageButton;
public:
    CViewLayer(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent)
    {
        m_Mode = MODE_LAYERS;
        m_LayerScrollValue = 0;
        m_ImageScrollValue = 0;
    }
    int GetType() { return EDITOR_VIEW_LAYER; }
    CUIRect Render(CUIRect *pView);

    void RenderLayers(CUIRect *pView);
    void RenderImages(CUIRect *pView);
};

class CViewEditor : public CView
{
    //ui handles
    void *m_uEditorView;
    int m_uZoomOutButton;
    int m_uZoomNormalButton;
    int m_uZoomInButton;

    //drag start
    float m_StartWx;
    float m_StartWy;


	bool m_GridActive;
	int m_GridFactor;

    float m_WorldOffsetX;
	float m_WorldOffsetY;
	float m_EditorOffsetX;
	float m_EditorOffsetY;
	float m_WorldZoom;
	int m_ZoomLevel;
	bool m_LockMouse;
	bool m_ShowMousePointer;
	bool m_GuiActive;
	bool m_ProofBorders;
	float m_MouseDeltaX;
	float m_MouseDeltaY;
	float m_MouseDeltaWx;
	float m_MouseDeltaWy;

	bool m_ShowTileInfo;
	bool m_ShowDetail;
	bool m_Animate;
	int64 m_AnimateStart;
	float m_AnimateTime;
	float m_AnimateSpeed;

    bool m_ShowPicker;
    int m_ShowEnvelopePreview; //Values: 0-Off|1-Selected Envelope|2-All

	int m_Operation;

public:
    CViewEditor(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent)
    {
        m_uEditorView = &m_uEditorView;

        m_StartWx = 0;
        m_StartWy = 0;

        m_ShowPicker = false;

		m_GridActive = false;
		m_GridFactor = 1;

		m_WorldOffsetX = 0;
		m_WorldOffsetY = 0;
		m_EditorOffsetX = 0.0f;
		m_EditorOffsetY = 0.0f;

		m_WorldZoom = 2.0f;
		m_ZoomLevel = 200;
		m_LockMouse = false;
		m_ShowMousePointer = true;

		m_GuiActive = true;
		m_ProofBorders = false;

		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
		m_MouseDeltaWx = 0;
		m_MouseDeltaWy = 0;

		m_ShowTileInfo = false;
		m_ShowDetail = true;
		m_Animate = false;
		m_AnimateStart = 0;
		m_AnimateTime = 0;
		m_AnimateSpeed = 1;

		m_ShowEnvelopePreview = 0;
		//m_SelectedQuadEnvelope = -1;
		//m_SelectedEnvelopePoint = -1;

		m_Operation = OP_EDITOR_NONE;

		m_GuiActive = true; //same?
		m_Toolbar = true;
    }
    int GetType() { return EDITOR_VIEW_EDITOR; }
    CUIRect Render(CUIRect *pView);
    void DoToolbar(CUIRect ToolBar);

    bool m_Toolbar;
};

class CViewScript : public CView
{
public:
    CViewScript(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent) {}
    int GetType() { return EDITOR_VIEW_SCRIPT; }
    CUIRect Render(CUIRect *pView);
};

class CViewCurves : public CView
{
public:
    CViewCurves(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent) {}
    int GetType() { return EDITOR_VIEW_CURVES; }
    CUIRect Render(CUIRect *pView);
};

struct CNode
{
    array<CNode *> m_lpInputs;
    array<CNode *> m_lpOutputs;
};

struct CNodeGroup
{
    array<CNode> m_lNodes;
    char m_aName[128];
};

class CViewNodes : public CView
{
    int m_uMenuPrev;
    int m_uMenuName;
    int m_uMenuNext;
    int m_uMenuAdd;
    char m_aMenuName[128];
    float m_aMenuNameOffset;

    int m_SelectedNodeGroup;

    array<CNodeGroup> m_lNodeGroups;
public:
    CViewNodes(class CEditor *pEditor, CViewGroup *pParent = 0) : CView(pEditor, pParent)
    {
        m_aMenuName[0] = 0;
        m_aMenuNameOffset = 0;
    }
    int GetType() { return EDITOR_VIEW_NODES; }
    float GetMinWidth() { return 150; }
    CUIRect Render(CUIRect *pView);
};

class CEditor : public IEditor
{
    friend class CView;
    friend class CViewGroup;
	class IInput *m_pInput;
	class IClient *m_pClient;
	class IConsole *m_pConsole;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IStorage *m_pStorage;
	CRenderTools m_RenderTools;
	CUI m_UI;

	CViewGroup *m_pMain;
	CView *m_pDragging;
    vec2 m_StartPos;
public:
	class IInput *Input() { return m_pInput; };
	class IClient *Client() { return m_pClient; };
	class IConsole *Console() { return m_pConsole; };
	class IGraphics *Graphics() { return m_pGraphics; };
	class ITextRender *TextRender() { return m_pTextRender; };
	class IStorage *Storage() { return m_pStorage; };
	CUI *UI() { return &m_UI; }
	CRenderTools *RenderTools() { return &m_RenderTools; }

	CEditor() : m_TilesetPicker(16, 16)
	{
		m_pInput = 0;
		m_pClient = 0;
		m_pGraphics = 0;
		m_pTextRender = 0;

		m_Mode = MODE_LAYERS;
		m_Dialog = 0;
		m_EditBoxActive = 0;
		m_pTooltip = 0;

		m_GridActive = false;
		m_GridFactor = 1;

		m_aFileName[0] = 0;
		m_aFileSaveName[0] = 0;
		m_ValidSaveFilename = false;

		m_PopupEventActivated = false;
		m_PopupEventWasActivated = false;

		m_FileDialogStorageType = 0;
		m_pFileDialogTitle = 0;
		m_pFileDialogButtonText = 0;
		m_pFileDialogUser = 0;
		m_aFileDialogFileName[0] = 0;
		m_aFileDialogCurrentFolder[0] = 0;
		m_aFileDialogCurrentLink[0] = 0;
		m_pFileDialogPath = m_aFileDialogCurrentFolder;
		m_aFileDialogActivate = false;
		m_FileDialogScrollValue = 0.0f;
		m_FilesSelectedIndex = -1;
		m_FilesStartAt = 0;
		m_FilesCur = 0;
		m_FilesStopAt = 999;

		m_WorldOffsetX = 0;
		m_WorldOffsetY = 0;
		m_EditorOffsetX = 0.0f;
		m_EditorOffsetY = 0.0f;

		m_WorldZoom = 1.0f;
		m_ZoomLevel = 200;
		m_LockMouse = false;
		m_ShowMousePointer = true;
		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
		m_MouseDeltaWx = 0;
		m_MouseDeltaWy = 0;

		m_GuiActive = true;
		m_ProofBorders = false;

		m_ShowTileInfo = false;
		m_ShowDetail = true;
		m_Animate = false;
		m_AnimateStart = 0;
		m_AnimateTime = 0;
		m_AnimateSpeed = 1;

		m_ShowEnvelopeEditor = 0;

		m_ShowEnvelopePreview = 0;
		m_SelectedQuadEnvelope = -1;
		m_SelectedEnvelopePoint = -1;

		ms_CheckerTexture = 0;
		ms_BackgroundTexture = 0;
		ms_CursorTexture = 0;
		ms_EntitiesTexture = 0;
		ms_UIButtons = 0;

		ms_pUiGotContext = 0;

		m_pDragging = 0;
		m_pMain = new CViewGroup(this);
	}

	virtual void Init();
	virtual void UpdateAndRender();
	virtual bool HasUnsavedData() { return m_Map.m_Modified; }

	void FilelistPopulate(int StorageType);
	void InvokeFileDialog(int StorageType, int FileType, const char *pTitle, const char *pButtonText,
		const char *pBasepath, const char *pDefaultName,
		void (*pfnFunc)(const char *pFilename, int StorageType, void *pUser), void *pUser);

	void Reset(bool CreateDefault=true);
	int Save(const char *pFilename);
	int Load(const char *pFilename, int StorageType);
	int Append(const char *pFilename, int StorageType);
	void Render();

	CQuad *GetSelectedQuad();
	CLayer *GetSelectedLayerType(int Index, int Type);
	CLayer *GetSelectedLayer(int Index);
	CLayerGroup *GetSelectedGroup();

	int DoProperties(CUIRect *pToolbox, CProperty *pProps, int *pIDs, int *pNewVal);

	int m_Mode;
	int m_Dialog;
	int m_EditBoxActive;
	const char *m_pTooltip;

	bool m_GridActive;
	int m_GridFactor;

	char m_aFileName[512];
	char m_aFileSaveName[512];
	bool m_ValidSaveFilename;

	enum
	{
		POPEVENT_EXIT=0,
		POPEVENT_LOAD,
		POPEVENT_NEW,
		POPEVENT_SAVE,
	};

	int m_PopupEventType;
	int m_PopupEventActivated;
	int m_PopupEventWasActivated;

	enum
	{
		FILETYPE_MAP,
		FILETYPE_IMG,

		MAX_PATH_LENGTH = 512
	};

	int m_FileDialogStorageType;
	const char *m_pFileDialogTitle;
	const char *m_pFileDialogButtonText;
	void (*m_pfnFileDialogFunc)(const char *pFileName, int StorageType, void *pUser);
	void *m_pFileDialogUser;
	char m_aFileDialogFileName[MAX_PATH_LENGTH];
	char m_aFileDialogCurrentFolder[MAX_PATH_LENGTH];
	char m_aFileDialogCurrentLink[MAX_PATH_LENGTH];
	char *m_pFileDialogPath;
	bool m_aFileDialogActivate;
	int m_FileDialogFileType;
	float m_FileDialogScrollValue;
	int m_FilesSelectedIndex;
	char m_FileDialogNewFolderName[64];
	char m_FileDialogErrString[64];

	struct CFilelistItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		bool m_IsLink;
		int m_StorageType;

		bool operator<(const CFilelistItem &Other) { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0; }
	};
	sorted_array<CFilelistItem> m_FileList;
	int m_FilesStartAt;
	int m_FilesCur;
	int m_FilesStopAt;

	float m_WorldOffsetX;
	float m_WorldOffsetY;
	float m_EditorOffsetX;
	float m_EditorOffsetY;
	float m_WorldZoom;
	int m_ZoomLevel;
	bool m_LockMouse;
	bool m_ShowMousePointer;
	bool m_GuiActive;
	bool m_ProofBorders;
	float m_MouseDeltaX;
	float m_MouseDeltaY;
	float m_MouseDeltaWx;
	float m_MouseDeltaWy;

	bool m_ShowTileInfo;
	bool m_ShowDetail;
	bool m_Animate;
	int64 m_AnimateStart;
	float m_AnimateTime;
	float m_AnimateSpeed;

	int m_ShowEnvelopeEditor;
	int m_ShowEnvelopePreview; //Values: 0-Off|1-Selected Envelope|2-All
	bool m_ShowPicker;

	int m_SelectedLayer;
	int m_SelectedGroup;
	int m_SelectedQuad;
	int m_SelectedPoints;
	int m_SelectedEnvelope;
	int m_SelectedEnvelopePoint;
    int m_SelectedQuadEnvelope;
	int m_SelectedImage;

	static int ms_CheckerTexture;
	static int ms_BackgroundTexture;
	static int ms_CursorTexture;
	static int ms_EntitiesTexture;
	static int ms_UIButtons;

	CLayerGroup m_Brush;
	CLayerTiles m_TilesetPicker;

	static const void *ms_pUiGotContext;

	CEditorMap m_Map;

	static void EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser);

	void DoMapBorder();
	int DoButton_Editor_Common(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Editor(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_Tab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Ex(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip, int Corners, float FontSize=10.0f);
	int DoButton_ButtonDec(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_ButtonInc(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

    //ui stuff
	int DoButton_UI(const void *pID, int Image, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Divider(const void *pID, const CUIRect *pRect);

	int DoButton_File(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_MenuItem(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags=0, const char *pToolTip=0);

	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden=false, int Corners=CUI::CORNER_ALL);

	void RenderBackground(CUIRect View, int Texture, float Size, float Brightness);

	void RenderGrid(CLayerGroup *pGroup);

	void UiInvokePopupMenu(void *pID, int Flags, float X, float Y, float W, float H, int (*pfnFunc)(CEditor *pEditor, CUIRect Rect, void *pUser), void *pExtra=0);
	void UiDoPopupMenu();

	int UiDoValueSelector(void *pID, CUIRect *pRect, const char *pLabel, int Current, int Min, int Max, int Step, float Scale, const char *pToolTip);

	static int PopupGroup(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupLayer(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupQuad(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupPoint(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupNewFolder(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupMapInfo(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupEvent(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupSelectImage(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupSelectGametileOp(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupImage(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupMenuFile(CEditor *pEditor, CUIRect View, void *pUser);
	static int PopupSelectConfigAutoMap(CEditor *pEditor, CUIRect View, void *pUser);

	static void CallbackOpenMap(const char *pFileName, int StorageType, void *pUser);
	static void CallbackAppendMap(const char *pFileName, int StorageType, void *pUser);
	static void CallbackSaveMap(const char *pFileName, int StorageType, void *pUser);

	void PopupSelectImageInvoke(int Current, float x, float y);
	int PopupSelectImageResult();

	void PopupSelectGametileOpInvoke(float x, float y);
	int PopupSelectGameTileOpResult();

	void PopupSelectConfigAutoMapInvoke(float x, float y);
	int PopupSelectConfigAutoMapResult();

	vec4 ButtonColorMul(const void *pID);

	void DoQuadEnvelopes(const array<CQuad> &m_lQuads, int TexID = -1);
	void DoQuadEnvPoint(const CQuad *pQuad, int QIndex, int pIndex);
	void DoQuadPoint(CQuad *pQuad, int QuadIndex, int v);

	void DoMapEditor(CUIRect View, CUIRect Toolbar);
	void DoToolbar(CUIRect Toolbar);
	void DoQuad(CQuad *pQuad, int Index);
	float UiDoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	vec4 GetButtonColor(const void *pID, int Checked);

	static void ReplaceImage(const char *pFilename, int StorageType, void *pUser);
	static void AddImage(const char *pFilename, int StorageType, void *pUser);

	void RenderImages(CUIRect Toolbox, CUIRect Toolbar, CUIRect View);
	void RenderLayers(CUIRect Toolbox, CUIRect Toolbar, CUIRect View);
	void RenderModebar(CUIRect View);
	void RenderStatusbar(CUIRect View);
	void RenderEnvelopeEditor(CUIRect View);

	void RenderFileDialog();

	void AddFileDialogEntry(int Index, CUIRect *pView);
	void SortImages();
	static void ExtractName(const char *pFileName, char *pName, int BufferSize)
	{
		const char *pExtractedName = pFileName;
		const char *pEnd = 0;
		for(; *pFileName; ++pFileName)
		{
			if(*pFileName == '/' || *pFileName == '\\')
				pExtractedName = pFileName+1;
			else if(*pFileName == '.')
				pEnd = pFileName;
		}

		int Length = pEnd > pExtractedName ? min(BufferSize, (int)(pEnd-pExtractedName+1)) : BufferSize;
		str_copy(pName, pExtractedName, Length);
	}

	int GetLineDistance();
};

// make sure to inline this function
inline class IGraphics *CLayer::Graphics() { return m_pEditor->Graphics(); }
inline class ITextRender *CLayer::TextRender() { return m_pEditor->TextRender(); }

#endif
