namespace SV {
	class Engine;
	class EngineDevice {
		Engine* m_pEngine = nullptr;
	public:
		inline void SetEngine(Engine* pEngine) noexcept { m_pEngine = pEngine; }
		inline Engine& GetEngine() const noexcept { return *m_pEngine; }
	};
}