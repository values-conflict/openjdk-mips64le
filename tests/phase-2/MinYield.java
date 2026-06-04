public class MinYield {
	public static void main(String[] args) throws Exception {
		Thread.ofVirtual().start(Thread::yield).join();
		System.out.println("ok");
	}
}
