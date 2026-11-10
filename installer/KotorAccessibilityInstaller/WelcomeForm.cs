using System;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;

namespace KotorAccessibilityInstaller
{
    public class WelcomeForm : Form
    {
        private Label _titleLabel;
        private Label _descriptionLabel;
        private Label _versionLabel;
        private Button _nextButton;

        private Label _gameTitleLabel;
        private Label _gameDescriptionLabel;
        private Button _backButton;
        private Button _installButton;

        private Panel _page1;
        private Panel _page2;

        public bool ProceedWithInstall { get; private set; } = false;
        public string LatestModVersion { get; set; }

        public WelcomeForm()
        {
            InitializeComponents();
            ApplyLocale();
        }

        private void InitializeComponents()
        {
            Text = Config.DisplayName;
            Size = new Size(520, 360);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;

            _page1 = new Panel { Location = new Point(0, 0), Size = new Size(520, 360), Visible = true };

            _titleLabel = new Label
            {
                Font = new Font(Font.FontFamily, 14, FontStyle.Bold),
                Location = new Point(20, 20),
                Size = new Size(470, 30),
                TextAlign = ContentAlignment.MiddleCenter
            };

            _descriptionLabel = new Label
            {
                Location = new Point(20, 60),
                Size = new Size(470, 90),
                TextAlign = ContentAlignment.TopLeft
            };

            _versionLabel = new Label
            {
                Location = new Point(20, 155),
                Size = new Size(470, 20),
                Font = new Font(Font.FontFamily, 9, FontStyle.Bold)
            };

            _nextButton = new Button { Location = new Point(350, 245), Size = new Size(140, 35) };
            _nextButton.Click += (s, e) => ShowPage2();

            _page1.Controls.AddRange(new Control[]
            {
                _titleLabel, _descriptionLabel, _versionLabel, _nextButton
            });

            // Page 2
            _page2 = new Panel { Location = new Point(0, 0), Size = new Size(520, 360), Visible = false };

            _gameTitleLabel = new Label
            {
                Font = new Font(Font.FontFamily, 14, FontStyle.Bold),
                Location = new Point(20, 20),
                Size = new Size(470, 30),
                TextAlign = ContentAlignment.MiddleCenter
            };

            _gameDescriptionLabel = new Label
            {
                Location = new Point(20, 60),
                Size = new Size(470, 110),
                TextAlign = ContentAlignment.TopLeft
            };

            // The store links used to live here. They moved to
            // GameStoreLinksForm, which runs AFTER the game-selection screen:
            // offering somewhere to buy a game before asking which game the user
            // wants is the wrong order, and this page only ever offered KOTOR 1.
            _backButton = new Button { Location = new Point(20, 245), Size = new Size(140, 35) };
            _backButton.Click += (s, e) => ShowPage1();

            _installButton = new Button { Location = new Point(350, 245), Size = new Size(140, 35) };
            _installButton.Click += (s, e) =>
            {
                ProceedWithInstall = true;
                Close();
            };

            _page2.Controls.AddRange(new Control[]
            {
                _gameTitleLabel, _gameDescriptionLabel, _backButton, _installButton
            });

            Controls.Add(_page1);
            Controls.Add(_page2);

            FormClosing += (s, e) =>
            {
                if (!ProceedWithInstall && !CancelConfirm.ConfirmCancel(this))
                {
                    e.Cancel = true;
                    return;
                }
                if (!ProceedWithInstall) Logger.Info("User closed welcome dialog without proceeding");
            };
        }

        private void ShowPage1()
        {
            _page2.Visible = false;
            _page1.Visible = true;
            UpdateAccessibleDescription();
            // Enter activates the page's primary action. The two pages have
            // different defaults, so this is re-pointed on every page switch.
            AcceptButton = _nextButton;
            _nextButton.Focus();
        }

        private void ShowPage2()
        {
            _page1.Visible = false;
            _page2.Visible = true;
            UpdateAccessibleDescription();
            AcceptButton = _installButton;
            _installButton.Focus();
        }

        // Escape cancels the installer from the welcome dialog. There is no
        // Cancel button to hang CancelButton off, and adding an invisible one
        // just to get Escape would put a phantom control in the screen
        // reader's view - so the key is handled directly.
        // ProceedWithInstall defaults to false, so closing here is already
        // treated as "cancelled from welcome dialog" by InstallFlow.
        protected override bool ProcessDialogKey(Keys keyData)
        {
            if (keyData == Keys.Escape)
            {
                Close();
                return true;
            }
            return base.ProcessDialogKey(keyData);
        }

        private void ApplyLocale()
        {
            _titleLabel.Text = InstallerLocale.Get("Welcome_Title");
            _descriptionLabel.Text = InstallerLocale.Get("Welcome_Description");
            _nextButton.Text = InstallerLocale.Get("Welcome_NextButton");
            _versionLabel.Text = !string.IsNullOrEmpty(LatestModVersion)
                ? InstallerLocale.Format("Welcome_VersionInfo_Format", LatestModVersion)
                : InstallerLocale.Get("Welcome_VersionInfo_Unknown");

            _gameTitleLabel.Text = InstallerLocale.Get("Welcome_GameTitle");
            _gameDescriptionLabel.Text = InstallerLocale.Get("Welcome_GameDescription");
            _backButton.Text = InstallerLocale.Get("Welcome_BackButton");
            _installButton.Text = InstallerLocale.Get("Welcome_NextButton");

            UpdateAccessibleDescription();
        }

        private void UpdateAccessibleDescription()
        {
            if (_page2 != null && _page2.Visible)
            {
                string body = $"{_gameTitleLabel.Text}. {_gameDescriptionLabel.Text}";
                AccessibleDescription = body;
                if (_installButton != null) _installButton.AccessibleDescription = body;
            }
            else
            {
                string version = _versionLabel != null ? _versionLabel.Text : string.Empty;
                string body = $"{_titleLabel.Text}. {_descriptionLabel.Text} {version}".Trim();
                AccessibleDescription = body;
                if (_nextButton != null) _nextButton.AccessibleDescription = body;
            }
        }
    }
}
