#region Using directives

using System;
using System.Runtime.InteropServices;
using System.Windows.Interop;

#endregion

namespace SentechCameraCoreWPF
{
    public class ControlHost : HwndHost
    {
        public SentechCameraCore.Wrapper m_sentechCameraCore;

        public int m_hostHeight;
        public int m_hostWidth;

        private IntPtr m_hwndHost;

        public ControlHost(double height, double width)
        {
            m_hostHeight = (int)height;
            m_hostWidth = (int)width;
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            m_hwndHost = IntPtr.Zero;
            m_sentechCameraCore = new SentechCameraCore.Wrapper();

            System.Windows.PresentationSource source = System.Windows.PresentationSource.FromVisual(this);

            unsafe
            {
                m_hwndHost = m_sentechCameraCore.Init(
                    m_hostHeight,
                    m_hostWidth,
                    (float)source.CompositionTarget.TransformToDevice.M11,
                    (float)source.CompositionTarget.TransformToDevice.M22,
                    hwndParent.Handle.ToPointer());
            }

            return new HandleRef(this, m_hwndHost);
        }

        protected override IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            handled = false;
            if(msg == 0x0005) // WM_SIZE
            {
                m_hostWidth = unchecked((short)lParam);
                m_hostHeight = unchecked((short)((uint)lParam >> 16));
                m_sentechCameraCore.Resize(m_hostHeight, m_hostWidth);
                handled = true;
            }
            return IntPtr.Zero;
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            DestroyWindow(hwnd.Handle);
        }

        [DllImport("user32.dll", EntryPoint = "DestroyWindow", CharSet = CharSet.Unicode)]
        internal static extern bool DestroyWindow(IntPtr hwnd);
    }
}
