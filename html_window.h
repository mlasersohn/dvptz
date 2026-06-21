
extern "C" {
int initialize_cef(int html, int argc, char **argv);
void shutdown_cef();
}

class RenderHandler: public CefRenderHandler
{
public:
	unsigned char *raw = NULL;
	int m_width = 0;
	int m_height = 0;
	int use_w = 0;
	int use_h = 0;
	int redraw_flag = 0;
	RenderHandler(int w, int h)
	{
		redraw_flag = 0;
		raw = (unsigned char *)malloc((size_t)w * (size_t)h  * 4);
		use_w = w;
		use_h = h;
		resize(w, h);
	}
	~RenderHandler()
	{
		if(raw != NULL)
		{
			free(raw);
			raw = NULL;
		}
	}
	virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override
	{
		rect = CefRect(0, 0, m_width, m_height);
	}
	virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int w, int h) override
	{
		resize(w, h);
		std::lock_guard<std::mutex> locker(m_mutex_texture);
		if((buffer == nullptr) || (w <= 0) || (h <= 0)) 
		{
			return;
		}
		if(raw != NULL)
		{
			if((w != use_w) || (h != use_h))
			{
				free(raw);
				raw = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
			}
			memcpy(raw, buffer, static_cast<size_t>((size_t)w * (size_t)h * 4));
			use_w = w;
			use_h = h;
			redraw_flag = 1;
		}
	}
	void resize(int w, int h)
	{
		std::lock_guard<std::mutex> locker(m_mutex_texture);
		if((w == m_width) && (h == m_height)) 
		{
			return;
		}
		if((w != use_w) || (h != use_h))
		{
			free(raw);
			raw = (unsigned char *)malloc((size_t)w * (size_t)h  * 4);
			use_w = w;
			use_h = h;
		}
		m_width = w;
		m_height = h;
	}
	void render()
	{
		std::lock_guard<std::mutex> locker(m_mutex_texture);
	}

private:
	std::mutex m_mutex_texture;

	IMPLEMENT_REFCOUNTING(RenderHandler);
};

class BrowserClient: public CefClient, public CefLifeSpanHandler, public CefLoadHandler, public CefDisplayHandler
{
public:
	BrowserClient(CefRefPtr<CefRenderHandler> ptr) : m_handler(ptr)
	{
		assert(ptr != nullptr);
	}
	~BrowserClient()
	{
	}
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
	{
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override
	{
		return this;
	}
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override
	{
		return m_handler;
	}
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
	{
		return this;
	}
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();
		m_browser_id = browser->GetIdentifier();
	}
	virtual bool DoClose(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();
		if(browser->GetIdentifier() == m_browser_id)
		{
			m_closing = true;
		}
		return false;
	}
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
	{
	}
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override
	{
		if(extra_css != NULL)
		{
			char cow[32768];
			if(strlen(extra_css) < 32000)
			{
				if(transparent_background == 1)
				{
					char cow[32768];
					sprintf(cow, "let styleSheetContent = ' * { background: transparent !important; } %s'; var style = document.createElement(\"style\"); style.type = \"text/css\"; style.id = \"cowcow\"; style.appendChild(document.createTextNode(styleSheetContent)); document.head.appendChild(style);", extra_css);
					browser->GetMainFrame()->ExecuteJavaScript(cow, "", 0);
					css_once = 1;
				}
				else
				{
					sprintf(cow, "let styleSheetContent = ' %s '; var style = document.createElement(\"style\"); style.type = \"text/css\"; style.id = \"cowcow\"; style.appendChild(document.createTextNode(styleSheetContent)); document.head.appendChild(style)", extra_css);
					css_once = 1;
				}
			}
		}
		else
		{
			if(transparent_background == 1)
			{
				char cow[32768];
				sprintf(cow, "let styleSheetContent = ' * { background: transparent !important; }'; var style = document.createElement(\"style\"); style.type = \"text/css\"; style.id = \"cowcow\"; style.appendChild(document.createTextNode(styleSheetContent)); document.head.appendChild(style);");
				browser->GetMainFrame()->ExecuteJavaScript(cow, "", 0);
				css_once = 1;
			}
		}
		m_loaded = true;
	}
	virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override
	{
		if(transparent_background == 0)
		{
			browser->GetMainFrame()->ExecuteJavaScript("document.body.style.backgroundColor='rgba(0, 0, 0, 0)';", "", 0);
		}
		if(js_once == 0)
		{
			if(extra_js_once != NULL)
			{
				browser->GetMainFrame()->ExecuteJavaScript(extra_js_once, "", 0);
			}
		}
		if(extra_js_always != NULL)
		{
			browser->GetMainFrame()->ExecuteJavaScript(extra_js_always, "", 0);
		}
		js_once = 1;
	}
	bool OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& failedUrl, CefString & errorText)
	{
		std::string str = errorText.ToString();
		if(str.c_str() != NULL)
		{
			printf("BROWSER ERROR: [%s]\n", str.c_str());
		}
		m_loaded = true;
		return true;
	}
	void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
	{
	}
	bool closeAllowed() const
	{
		return m_closing;
	}
	bool isLoaded() const
	{
		return m_loaded;
	}
	int		transparent_background = 0;
	char	*extra_css = NULL;
	char	*extra_js_once = NULL;
	char	*extra_js_always = NULL;
private:
	int m_browser_id = -1;
	std::atomic<bool> m_closing{false};
	std::atomic<bool> m_loaded{false};
	CefRefPtr<CefRenderHandler> m_handler = nullptr;
	int css_once = 0;
	int js_once = 0;

	IMPLEMENT_REFCOUNTING(BrowserClient);
};

class   HTML_Win
{
public:
		HTML_Win(char *in_url, char *in_html, int transparent_background, char *in_extra_css, char *in_extra_js_once, char *in_extra_js_always, int ww, int hh);
		~HTML_Win();

	void    Draw();
	int	initialize(int argc, char **argv);
	int	load(char *address, char *html_contents, int transparent_background, char *extra_css, char *extra_js_once, char *extra_js_always);
	int	shutdown();
	void	resize(int ww, int hh);

	CefRefPtr<RenderHandler> renderHandler;
	CefRefPtr<BrowserClient> browserClient;
	CefRefPtr<CefBrowser> browser;

	int	width;
	int	height;
	unsigned char *raw;
};
