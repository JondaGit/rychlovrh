import { useState } from "react";
import { AnimatePresence, motion } from "motion/react";
import { useDevice } from "./hooks/useDevice";
import { Home } from "./pages/Home";
import { Settings } from "./pages/Settings";

const PAGE_VARIANTS = {
  initial: (dir: number) => ({ opacity: 0, x: 24 * dir }),
  animate: { opacity: 1, x: 0 },
  exit: (dir: number) => ({ opacity: 0, x: -24 * dir }),
};

export default function App() {
  const [view, setView] = useState<"home" | "settings">("home");
  const device = useDevice();
  const direction = view === "home" ? -1 : 1;

  return (
    <div className="app">
      <AnimatePresence mode="wait" initial={false} custom={direction}>
        {view === "home" ? (
          <motion.div
            key="home"
            className="view"
            custom={direction}
            variants={PAGE_VARIANTS}
            initial="initial"
            animate="animate"
            exit="exit"
            transition={{ duration: 0.24, ease: [0.4, 0, 0.2, 1] }}
          >
            <Home device={device} onSettings={() => setView("settings")} />
          </motion.div>
        ) : (
          <motion.div
            key="settings"
            className="view"
            custom={direction}
            variants={PAGE_VARIANTS}
            initial="initial"
            animate="animate"
            exit="exit"
            transition={{ duration: 0.24, ease: [0.4, 0, 0.2, 1] }}
          >
            <Settings device={device} onBack={() => setView("home")} />
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
