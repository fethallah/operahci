package org.gnf.IO;

import java.io.File;
import java.net.URISyntaxException;

public class ResourcesResolver {
	final static String RESOURCES_BASE = "Resources";

	public ResourcesResolver() {
		// TODO Auto-generated constructor stub
	}

	public static File getResourceFile(String RelativePath) {

		RelativePath = String
				.format("/%1$s/%2$s", RESOURCES_BASE, RelativePath);
		File resourcesLocs;

		try {
			resourcesLocs = new File((new ResourcesResolver()).getClass()
					.getProtectionDomain().getCodeSource().getLocation()
					.toURI()).getParentFile();
			resourcesLocs = new File(resourcesLocs, RelativePath);
			if (resourcesLocs.exists() && resourcesLocs.canRead())
				return resourcesLocs;
		} catch (URISyntaxException e) {
			e.printStackTrace();
		}
		String filePath = (new ResourcesResolver()).getClass()
				.getResource(RelativePath).getPath();
		if (filePath != null)
			return new File(filePath);
		return null;
	}

	public static File getResourceFile(File RelativePath) {
		String path = RelativePath.getPath().replaceAll("\\\\", "/");
		return getResourceFile(path);
	}

	public static void main(String[] args) {
		System.err.print(ResourcesResolver.getResourceFile(new File(
				"\\ScriptPrefix")));
	}
}
