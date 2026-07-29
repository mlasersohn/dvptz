#define	DVPTZ_VERSION_NUMBER		"0.1.003"
#define	CURRENT_YEAR				2026

class	GenericPopupMenu : public Fl_Window
{
public:
					GenericPopupMenu(int xx, int yy, int ww, int hh);
					~GenericPopupMenu();
	int				handle(int event);
	void			show();
	void			hide();

	void			Resize(int xx, int yy, int ww, int hh);
	void			Fit();

	Fl_Hold_Browser	*browser;
	time_t			start_time;
};
